StatusOr<std::vector<std::vector<xla::XlaOp>>> GetTensorListDynamicDims(
    XlaOpKernelContext* ctx, const xla::Shape& element_shape,
    const xla::Shape& list_shape, int64_t num_elements) {
  std::vector<int64_t> dynamic_sizes;
  // The multiplier can be a dynamic value.
  TF_RETURN_IF_ERROR(ctx->ConstantInputAsIntVector(0, &dynamic_sizes));
  std::vector<bool> dims_are_dynamic;
  TF_RETURN_IF_ERROR(
      ctx->ResolveInputDynamismIntoPredVector(0, &dims_are_dynamic));
  bool leading_dim_is_dynamic;
  TF_RETURN_IF_ERROR(
      ctx->ResolveInputDynamismIntoPred(1, &leading_dim_is_dynamic));
  
  // Pre-allocate and initialize static dims first
  std::vector<xla::XlaOp> dynamic_dims;
  dynamic_dims.reserve(1 + element_shape.dimensions_size());
  
  // Handle leading dimension
  if (leading_dim_is_dynamic) {
    dynamic_dims.push_back(ctx->Input(1));
  } else {
    dynamic_dims.push_back(
        xla::ConstantR0<int32>(ctx->builder(), num_elements));
  }

  // Create all static dim constants in one batch
  std::vector<xla::XlaOp> static_dims;
  for (int64_t dim = 0; dim < element_shape.dimensions_size(); ++dim) {
    if (!dims_are_dynamic[dim]) {
      static_dims.push_back(
          xla::ConstantR0<int32>(ctx->builder(), dynamic_sizes[dim]));
    }
  }

  // Process all dimensions, reusing pre-built constants for static dims
  int static_idx = 0;
  for (int64_t dim = 0; dim < element_shape.dimensions_size(); ++dim) {
    if (dims_are_dynamic[dim]) {
      auto dynamic_dim_size = xla::Slice(ctx->Input(0), {dim}, {dim + 1}, {1});
      dynamic_dim_size = xla::Reshape(dynamic_dim_size, {});
      dynamic_dim_size = xla::ConvertElementType(dynamic_dim_size, xla::S32);
      dynamic_dims.push_back(dynamic_dim_size);
    } else {
      dynamic_dims.push_back(static_dims[static_idx++]);
    }
  }

  return {{dynamic_dims}};
}