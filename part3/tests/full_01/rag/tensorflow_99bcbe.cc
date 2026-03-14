Status MetaOptimizer::OptimizeConsumeItem(Cluster* cluster, GrapplerItem&& item,
                                          GraphDef* optimized_graph) {
  tensorflow::metrics::ScopedCounter<2> timings(
      tensorflow::metrics::GetGraphOptimizationCounter(),
      {kGrapplerCategory, "*"});

  VLOG(1) << "Starting optimization for grappler item: " << item.id;
  optimization_results_.clear();

  // Single pass function library minimization
  const auto minimized_flib =
      [](const GraphDef& graph) -> FunctionLibraryDefinition {
    return FunctionLibraryDefinition(OpRegistry::Global(), graph.library())
        .ReachableDefinitions(graph);
  };

  // Minimize function library in one pass
  int old_library_size = item.graph.library().function_size();
  *item.graph.mutable_library() = minimized_flib(item.graph).ToProto();
  int new_library_size = item.graph.library().function_size();
  VLOG(1) << absl::Substitute(
      "Deleted $0 unreachable functions from the graph (library size = $1)",
      old_library_size - new_library_size, new_library_size);

  bool optimize_function_library =
      item.optimization_options().optimize_function_library;
  const auto producer = item.graph.versions().producer();

  // Optimize main graph
  TF_RETURN_IF_ERROR(OptimizeGraph(cluster, std::move(item), optimized_graph));
  VLOG(1) << "Optimized main graph.";
  GRAPPLER_RETURN_IF_DEADLINE_EXCEEDED();

  // Combined function analysis
  FunctionLibraryDefinition flib = minimized_flib(*optimized_graph);
  absl::flat_hash_set<string> differentiable_functions;
  absl::flat_hash_set<string> xla_compiled_functions;
  absl::flat_hash_set<string> optimized_funcs;

  // Single pass to detect differentiable and XLA compiled functions
  const auto analyze_functions = [&](const NodeDef& node) {
    // Check for symbolic gradient
    if (IsSymbolicGradient(node)) {
      if (const auto* f_attr = gtl::FindOrNull(node.attr(), "f")) {
        differentiable_functions.insert(f_attr->func().name());
      }
    }
    // Check for XLA launch
    if (IsXlaLaunch(node)) {
      NameAttrList function;
      if (GetNodeAttr(node, "function", &function).ok()) {
        std::function<void(const string&)> mark_xla_func;
        mark_xla_func = [&](const string& func_name) {
          if (xla_compiled_functions.contains(func_name)) return;
          if (const FunctionDef* func_def = flib.Find(func_name)) {
            xla_compiled_functions.insert(func_name);
            for (const NodeDef& n : func_def->node_def()) {
              for (const auto& attr : n.attr()) {
                if (attr.second.has_func()) {
                  mark_xla_func(attr.second.func().name());
                }
              }
            }
          }
        };
        mark_xla_func(function.name());
      }
    }
  };

  // Analyze main graph nodes
  for (const NodeDef& node : optimized_graph->node()) {
    analyze_functions(node);
  }

  // Analyze function library nodes
  for (const FunctionDef& function : optimized_graph->library().function()) {
    for (const NodeDef& node : function.node_def()) {
      analyze_functions(node);
    }
  }

  // Propagate TF data attributes
  PropagateTFDataAttrs(flib, *optimized_graph->mutable_library());

  // Optimize functions in a single pass when possible
  while (optimize_function_library) {
    optimize_function_library = false;
    int function_idx = 0;

    for (const FunctionDef& func : optimized_graph->library().function()) {
      GRAPPLER_RETURN_IF_DEADLINE_EXCEEDED();
      const string& func_name = func.signature().name();

      // Skip conditions in a single check
      if (!flib.Contains(func_name) || optimized_funcs.contains(func_name) ||
          xla_compiled_functions.contains(func_name) || IsParametrized(func) ||
          data::IsTFDataFunction(func)) {
        continue;
      }

      VLOG(3) << "Optimize function: function=" << func_name << " ["
              << function_idx++ << " of "
              << optimized_graph->library().function_size() << "]";

      optimize_function_library = true;
      optimized_funcs.insert(func_name);

      // Process function optimization
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

      // Add new functions to library
      for (const FunctionDef& func_def :
           optimized_func_graph.library().function()) {
        if (!flib.Contains(func_def.signature().name())) {
          TF_RETURN_IF_ERROR(flib.AddFunctionDef(func_def));
        }
      }

      // Convert and replace optimized function
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