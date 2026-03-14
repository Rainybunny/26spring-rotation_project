Status TRTOptimizationPass::Optimize(grappler::Cluster* cluster,
                                     const grappler::GrapplerItem& item,
                                     GraphDef* optimized_graph) {
  VLOG(1) << "Called TRTOptimization Pass " << name_
          << " on a grappler item with id=" << item.id;
  TF_ASSIGN_OR_RETURN(bool do_function_conversion, ShouldConvertFunction(item));
  if ((minimum_segment_size_ == -1 && item.id == "tf_graph") ||
      (item.id != "tf_graph" && !do_function_conversion)) {
    VLOG(1) << "Not optimizing this grappler item: " << item.id;
    *optimized_graph = item.graph;
    return Status::OK();
  }
  if (VLOG_IS_ON(3)) {
    LOG(INFO) << CurrentStackTrace();
    PrintDebugInfo(cluster, item);
  }

  if (use_calibration_ && precision_mode_ != TrtPrecisionMode::INT8) {
    VLOG(1) << "Calibration with FP32 or FP16 is not implemented. "
            << "Falling back to use_calibration = False.";
    use_calibration_ = false;
  }

  const auto& nodes = item.NodesToPreserve();
  std::vector<string> nodes_to_preserve;
  nodes_to_preserve.reserve(nodes.size());

  for (const auto& n : nodes) {
    const auto tokens = str_util::Split(n, ":");
    if (tokens.empty()) continue;
    
    string s(tokens[0]);
    const size_t token_count = tokens.size();
    
    // Handle port number case
    if (token_count > 1) {
      int dummy_port = -1;
      const bool has_port = strings::safe_strto32(tokens.back(), &dummy_port);
      const size_t end = has_port ? token_count - 1 : token_count;
      
      for (size_t i = 1; i < end; ++i) {
        s.append(":").append(tokens[i]);
      }
    }
    nodes_to_preserve.emplace_back(std::move(s));
  }

  ConversionParams cp;
  cp.grappler_item = &item;
  cp.output_names = &nodes_to_preserve;
  cp.trt_logger_name = trt_logger_name_;
  cp.max_batch_size = maximum_batch_size_;
  cp.max_workspace_size_bytes = max_workspace_size_bytes_;
  cp.output_graph_def = optimized_graph;
  cp.precision_mode = precision_mode_;
  cp.minimum_segment_size = minimum_segment_size_;
  cp.cluster = cluster;
  cp.is_dyn_op = is_dynamic_op_;
  cp.max_cached_engines = max_cached_batches_;
  cp.use_calibration = use_calibration_;
  cp.use_implicit_batch = use_implicit_batch_;
  cp.profile_strategy = profile_strategy_;
  cp.allow_build_at_runtime = allow_build_at_runtime_;

  if (item.id != "tf_graph" && do_function_conversion) {
    const grappler::GrapplerFunctionItem& func_item =
        tensorflow::down_cast<const grappler::GrapplerFunctionItem&>(item);
    TF_RETURN_IF_ERROR(
        UpdateFunctionSpecificConversionParams(cp, func_item.func_attr()));
    assert(cp.minimum_segment_size > 0);
  }

  auto status = ConvertAfterShapes(cp);
  VLOG(1) << "Returning from " << name_;
  return status;
}