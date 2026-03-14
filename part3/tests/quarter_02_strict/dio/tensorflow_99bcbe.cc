Status MetaOptimizer::OptimizeConsumeItem(Cluster* cluster, GrapplerItem&& item,
                                          GraphDef* optimized_graph) {
  tensorflow::metrics::ScopedCounter<2> timings(
      tensorflow::metrics::GetGraphOptimizationCounter(),
      {kGrapplerCategory, "*"});

  VLOG(1) << "Starting optimization for grappler item: " << item.id;
  optimization_results_.clear();

  // Construct minimized function library
  int old_library_size = item.graph.library().function_size();
  *item.graph.mutable_library() = FunctionLibraryDefinition(
      OpRegistry::Global(), item.graph.library())
      .ReachableDefinitions(item.graph)
      .ToProto();
  int new_library_size = item.graph.library().function_size();
  VLOG(1) << absl::Substitute(
      "Deleted $0 unreachable functions from the graph (library size = $1)",
      old_library_size - new_library_size, new_library_size);

  bool optimize_function_library =
      item.optimization_options().optimize_function_library;
  const auto producer = item.graph.versions().producer();

  // 1. Optimize main graph
  TF_RETURN_IF_ERROR(OptimizeGraph(cluster, std::move(item), optimized_graph));
  VLOG(1) << "Optimized main graph.";
  GRAPPLER_RETURN_IF_DEADLINE_EXCEEDED();

  // 2. Optimize functions
  FunctionLibraryDefinition flib = FunctionLibraryDefinition(
      OpRegistry::Global(), optimized_graph->library());

  // Find differentiable functions
  absl::flat_hash_set<string> differentiable_functions;
  const auto find_diff_funcs = [&](const auto& nodes) {
    for (const NodeDef& node : nodes) {
      if (IsSymbolicGradient(node)) {
        if (const auto* f_attr = gtl::FindOrNull(node.attr(), "f")) {
          differentiable_functions.insert(f_attr->func().name());
        }
      }
    }
  };
  find_diff_funcs(optimized_graph->node());
  for (const FunctionDef& function : optimized_graph->library().function()) {
    find_diff_funcs(function.node_def());
  }

  // Find XLA compiled functions
  absl::flat_hash_set<string> xla_compiled_functions;
  std::function<void(const string&)> find_all_funcs = [&](const string& func) {
    if (xla_compiled_functions.contains(func)) return;
    if (const FunctionDef* func_def = flib.Find(func)) {
      xla_compiled_functions.insert(func);
      for (const NodeDef& node : func_def->node_def()) {
        for (const auto& attr : node.attr()) {
          if (attr.second.has_func()) {
            find_all_funcs(attr.second.func().name());
          }
        }
      }
    }
  };
  const auto find_xla_funcs = [&](const auto& nodes) {
    NameAttrList function;
    for (const NodeDef& node : nodes) {
      if (IsXlaLaunch(node) && GetNodeAttr(node, "function", &function).ok()) {
        find_all_funcs(function.name());
      }
    }
  };
  find_xla_funcs(optimized_graph->node());
  for (const FunctionDef& function : optimized_graph->library().function()) {
    find_xla_funcs(function.node_def());
  }
  PropagateTFDataAttrs(flib, *optimized_graph->mutable_library());

  // Parallel function optimization
  absl::flat_hash_set<string> optimized_funcs;
  std::mutex mutex;
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
    auto optimize_function = [&](const FunctionDef* func) {
      const string& func_name = func->signature().name();
      VLOG(3) << "Optimize function: " << func_name;

      GrapplerFunctionItem func_item;
      Status status = MakeGrapplerFunctionItem(*func, flib, producer, &func_item);
      if (!status.ok()) return status;

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
        status = implementation_selector.Optimize(cluster, func_item,
                                                 &optimized_func_graph);
      } else {
        GrapplerFunctionItem func_item_copy = func_item;
        status = OptimizeGraph(cluster, std::move(func_item_copy),
                              &optimized_func_graph);
      }
      if (!status.ok()) return status;

      // Add new functions to library
      {
        std::lock_guard<std::mutex> lock(mutex);
        for (const FunctionDef& func_def :
             optimized_func_graph.library().function()) {
          if (!flib.Contains(func_def.signature().name())) {
            TF_RETURN_IF_ERROR(flib.AddFunctionDef(func_def));
          }
        }
      }

      // Convert back to FunctionDef
      FunctionDef optimized_func;
      func_item.SwapFunctionBody(std::move(optimized_func_graph));
      TF_RETURN_IF_ERROR(MakeFunctionDef(func_item, flib, &optimized_func));

      // Replace function
      std::lock_guard<std::mutex> lock(mutex);
      TF_RETURN_IF_ERROR(flib.ReplaceFunction(func_name, optimized_func));
      optimized_funcs.insert(func_name);
      return Status::OK();
    };

    // Process functions in parallel
    std::vector<Status> statuses(functions_to_optimize.size());
    {
      tensorflow::thread::ThreadPool pool(
          Env::Default(), "optimize_functions", 
          std::min(8, (int)functions_to_optimize.size()));
      for (int i = 0; i < functions_to_optimize.size(); ++i) {
        pool.Schedule([&, i]() {
          statuses[i] = optimize_function(functions_to_optimize[i]);
        });
      }
    }

    // Check for errors
    for (const Status& status : statuses) {
      TF_RETURN_IF_ERROR(status);
    }

    optimize_function_library = !functions_to_optimize.empty();
    if (optimize_function_library) {
      *optimized_graph->mutable_library() = flib.ToProto();
    }
  }

  VLOG(1) << "Optimized " << optimized_funcs.size() << " functions";
  return Status::OK();
}