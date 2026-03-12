Status MetaOptimizer::OptimizeConsumeItem(Cluster* cluster, GrapplerItem&& item,
                                          GraphDef* optimized_graph) {
  tensorflow::metrics::ScopedCounter<2> timings(
      tensorflow::metrics::GetGraphOptimizationCounter(),
      {kGrapplerCategory, "*"});

  VLOG(1) << "Starting optimization for grappler item: " << item.id;
  optimization_results_.clear();

  const auto minimized_flib =
      [](const GraphDef& graph) -> FunctionLibraryDefinition {
    return FunctionLibraryDefinition(OpRegistry::Global(), graph.library())
        .ReachableDefinitions(graph);
  };

  int old_library_size = item.graph.library().function_size();
  *item.graph.mutable_library() = minimized_flib(item.graph).ToProto();
  int new_library_size = item.graph.library().function_size();

  VLOG(1) << absl::Substitute(
      "Deleted $0 unreachable functions from the graph (library size = $1)",
      old_library_size - new_library_size, new_library_size);

  bool optimize_function_library =
      item.optimization_options().optimize_function_library;
  const auto producer = item.graph.versions().producer();

  TF_RETURN_IF_ERROR(OptimizeGraph(cluster, std::move(item), optimized_graph));
  VLOG(1) << "Optimized main graph.";
  GRAPPLER_RETURN_IF_DEADLINE_EXCEEDED();

  FunctionLibraryDefinition flib = minimized_flib(*optimized_graph);
  using NodeDefs = protobuf::RepeatedPtrField<NodeDef>;

  // Pre-allocate hash sets based on function library size
  absl::flat_hash_set<string> differentiable_functions;
  differentiable_functions.reserve(flib.num_functions());

  const auto find_differentiable_functions =
      [&](const NodeDefs& nodes) -> void {
    for (const NodeDef& node : nodes) {
      if (IsSymbolicGradient(node)) {
        const auto* f_attr = gtl::FindOrNull(node.attr(), "f");
        if (f_attr) differentiable_functions.insert(f_attr->func().name());
      }
    }
  };

  find_differentiable_functions(optimized_graph->node());
  for (const FunctionDef& function : optimized_graph->library().function()) {
    find_differentiable_functions(function.node_def());
  }

  // Pre-allocate xla_compiled_functions set
  absl::flat_hash_set<string> xla_compiled_functions;
  xla_compiled_functions.reserve(flib.num_functions());

  std::function<void(const string&)> find_all_functions;
  find_all_functions = [&](const string& func) -> void {
    if (xla_compiled_functions.contains(func)) return;
    const FunctionDef* func_def = flib.Find(func);
    CHECK(func_def) << "not found: " << func;
    xla_compiled_functions.insert(func);
    for (const NodeDef& node : func_def->node_def()) {
      for (const auto attr : node.attr()) {
        const AttrValue& attr_value = attr.second;
        if (attr_value.has_func()) {
          find_all_functions(attr_value.func().name());
        }
      }
    }
  };

  auto find_xla_compiled_functions = [&](const NodeDefs& nodes) -> void {
    NameAttrList function;
    for (const NodeDef& node : nodes) {
      if (!IsXlaLaunch(node)) continue;
      if (!GetNodeAttr(node, "function", &function).ok()) continue;
      find_all_functions(function.name());
    }
  };

  find_xla_compiled_functions(optimized_graph->node());
  for (const FunctionDef& function : optimized_graph->library().function()) {
    find_xla_compiled_functions(function.node_def());
  }
  PropagateTFDataAttrs(flib, *optimized_graph->mutable_library());

  // Pre-allocate optimized_funcs set
  absl::flat_hash_set<string> optimized_funcs;
  optimized_funcs.reserve(flib.num_functions());

  while (optimize_function_library) {
    optimize_function_library = false;

    int function_idx = 0;
    for (const FunctionDef& func : optimized_graph->library().function()) {
      GRAPPLER_RETURN_IF_DEADLINE_EXCEEDED();

      const string& func_name = func.signature().name();
      if (!flib.Contains(func_name)) continue;
      if (optimized_funcs.contains(func_name)) continue;
      if (xla_compiled_functions.contains(func_name)) continue;
      if (IsParametrized(func)) continue;
      if (data::IsTFDataFunction(func)) continue;

      VLOG(3) << "Optimize function: function=" << func_name << " ["
              << function_idx++ << " of "
              << optimized_graph->library().function_size() << "]";

      optimize_function_library = true;
      optimized_funcs.insert(func_name);

      GrapplerFunctionItem func_item;
      TF_RETURN_IF_ERROR(
          MakeGrapplerFunctionItem(func, flib, producer, &func_item));

      func_item.optimization_options().allow_non_differentiable_rewrites =
          !differentiable_functions.contains(func_name);

      if (!func_item.devices().empty()) {
        return errors::Internal("GrapplerFunctionItem devices must be empty.");
      }

      func_item.optimization_options().allow_pruning_stateful_and_dataset_ops =
          false;

      GraphDef optimized_func_graph;
      if (IsTPUGraphDef(*optimized_graph)) {
        ImplementationSelector implementation_selector;
        std::unique_ptr<FunctionDefLibrary> func_item_function_library(
            func_item.graph.release_library());
        *func_item.graph.mutable_library() =
            GetFunctionDefLibraryStub(*func_item_function_library);
        TF_RETURN_IF_ERROR(implementation_selector.Optimize(
            cluster, func_item, &optimized_func_graph));
      } else {
        GrapplerFunctionItem func_item_copy = func_item;
        TF_RETURN_IF_ERROR(OptimizeGraph(cluster, std::move(func_item_copy),
                                         &optimized_func_graph));
      }

      for (const FunctionDef& func_def :
           optimized_func_graph.library().function()) {
        if (flib.Find(func_def.signature().name()) == nullptr) {
          TF_RETURN_IF_ERROR(flib.AddFunctionDef(func_def));
        }
      }

      FunctionDef optimized_func;
      func_item.SwapFunctionBody(std::move(optimized_func_graph));
      TF_RETURN_IF_ERROR(MakeFunctionDef(func_item, flib, &optimized_func));
      TF_RETURN_IF_ERROR(flib.ReplaceFunction(func_name, optimized_func));
    }

    if (optimize_function_library) {
      *optimized_graph->mutable_library() = flib.ToProto();
    }
  }

  VLOG(1) << "Optimized " << optimized_funcs.size()
          << " functions: " << absl::StrJoin(optimized_funcs, ", ");
  VLOG(3) << "Optimized graph =\n" << optimized_graph->DebugString();
  if (VLOG_IS_ON(1)) {
    DumpGraphDefToFile(
        strings::StrCat("after_MetaOptimizer_",
                        reinterpret_cast<uintptr_t>(optimized_graph)),
        *optimized_graph);
  }

  return Status::OK();
}