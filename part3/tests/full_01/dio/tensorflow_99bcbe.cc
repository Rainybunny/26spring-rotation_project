Status MetaOptimizer::OptimizeConsumeItem(Cluster* cluster, GrapplerItem&& item,
                                          GraphDef* optimized_graph) {
  tensorflow::metrics::ScopedCounter<2> timings(
      tensorflow::metrics::GetGraphOptimizationCounter(),
      {kGrapplerCategory, "*"});

  VLOG(1) << "Starting optimization for grappler item: " << item.id;
  optimization_results_.clear();

  // Constructs a FunctionLibraryDefinition with functions that are reachable
  // from the nodes of the graph.
  const auto minimized_flib =
      [](const GraphDef& graph) -> FunctionLibraryDefinition {
    return FunctionLibraryDefinition(OpRegistry::Global(), graph.library())
        .ReachableDefinitions(graph);
  };

  // 0. Original graph might contain a huge function library, that is mostly
  // unused. This library copied over by each individual Grappler optimizer,
  // which adds a huge overhead. Before starting optimization passes we just
  // remove all the unreachable functions.
  int old_library_size = item.graph.library().function_size();
  *item.graph.mutable_library() = minimized_flib(item.graph).ToProto();
  int new_library_size = item.graph.library().function_size();

  VLOG(1) << absl::Substitute(
      "Deleted $0 unreachable functions from the graph (library size = $1)",
      old_library_size - new_library_size, new_library_size);

  // Save a few small fields from item before we move it.
  bool optimize_function_library =
      item.optimization_options().optimize_function_library;
  const auto producer = item.graph.versions().producer();

  // 1. Optimize main graph
  TF_RETURN_IF_ERROR(OptimizeGraph(cluster, std::move(item), optimized_graph));
  VLOG(1) << "Optimized main graph.";
  GRAPPLER_RETURN_IF_DEADLINE_EXCEEDED();

  // 2. Optimize functions reachable from the optimized graph.
  FunctionLibraryDefinition flib = minimized_flib(*optimized_graph);
  using NodeDefs = protobuf::RepeatedPtrField<NodeDef>;

  // Find functions for which we might need to compute a gradient at runtime.
  absl::flat_hash_set<string> differentiable_functions;
  const auto find_differentiable_functions =
      [&](const NodeDefs& nodes) -> void {
    for (const NodeDef& node : nodes) {
      if (IsSymbolicGradient(node)) {
        const auto* f_attr = gtl::FindOrNull(node.attr(), "f");
        if (f_attr) differentiable_functions.insert(f_attr->func().name());
      }
    }
  };

  // SymbolicGradient nodes inside the main graph and function library
  find_differentiable_functions(optimized_graph->node());
  for (const FunctionDef& function : optimized_graph->library().function()) {
    find_differentiable_functions(function.node_def());
  }

  // Find functions that will be compiled by XLA later
  absl::flat_hash_set<string> xla_compiled_functions;
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

  // Optimize each function only once.
  absl::flat_hash_set<string> optimized_funcs;
  while (optimize_function_library) {
    optimize_function_library = false;

    // Collect functions to optimize
    std::vector<const FunctionDef*> functions_to_optimize;
    for (const FunctionDef& func : optimized_graph->library().function()) {
      const string& func_name = func.signature().name();
      if (!flib.Contains(func_name) || optimized_funcs.contains(func_name) ||
          xla_compiled_functions.contains(func_name) || IsParametrized(func) ||
          data::IsTFDataFunction(func)) {
        continue;
      }
      functions_to_optimize.push_back(&func);
    }

    // Parallel optimization of functions
    absl::Mutex mutex;
    std::vector<Status> statuses(functions_to_optimize.size());
    auto optimize_function = [&](int i) {
      const FunctionDef* func = functions_to_optimize[i];
      const string& func_name = func->signature().name();

      VLOG(3) << "Optimize function: function=" << func_name << " [" << i
              << " of " << functions_to_optimize.size() << "]";

      GrapplerFunctionItem func_item;
      Status s = MakeGrapplerFunctionItem(*func, flib, producer, &func_item);
      if (!s.ok()) {
        statuses[i] = s;
        return;
      }

      func_item.optimization_options().allow_non_differentiable_rewrites =
          !differentiable_functions.contains(func_name);
      func_item.optimization_options().allow_pruning_stateful_and_dataset_ops =
          false;

      GraphDef optimized_func_graph;
      if (IsTPUGraphDef(*optimized_graph)) {
        ImplementationSelector implementation_selector;
        std::unique_ptr<FunctionDefLibrary> func_item_function_library(
            func_item.graph.release_library());
        *func_item.graph.mutable_library() =
            GetFunctionDefLibraryStub(*func_item_function_library);
        statuses[i] = implementation_selector.Optimize(
            cluster, func_item, &optimized_func_graph);
      } else {
        GrapplerFunctionItem func_item_copy = func_item;
        statuses[i] = OptimizeGraph(cluster, std::move(func_item_copy),
                                   &optimized_func_graph);
      }

      if (statuses[i].ok()) {
        FunctionDef optimized_func;
        func_item.SwapFunctionBody(std::move(optimized_func_graph));
        statuses[i] = MakeFunctionDef(func_item, flib, &optimized_func);
        if (statuses[i].ok()) {
          absl::MutexLock lock(&mutex);
          TF_RETURN_IF_ERROR(flib.ReplaceFunction(func_name, optimized_func));
          optimized_funcs.insert(func_name);
          optimize_function_library = true;
        }
      }
    };

    // Use parallel execution with number of threads equal to available cores
    const int num_threads = port::MaxParallelism();
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int t = 0; t < num_threads; ++t) {
      threads.emplace_back([&, t]() {
        for (int i = t; i < functions_to_optimize.size(); i += num_threads) {
          GRAPPLER_RETURN_IF_DEADLINE_EXCEEDED();
          optimize_function(i);
        }
      });
    }
    for (auto& thread : threads) {
      thread.join();
    }

    // Check for any errors
    for (const Status& s : statuses) {
      TF_RETURN_IF_ERROR(s);
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