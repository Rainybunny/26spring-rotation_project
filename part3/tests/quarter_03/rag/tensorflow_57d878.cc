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

  // Cache inputs to avoid repeated access
  xla::XlaOp input0 = ctx->Input(0);
  xla::XlaOp input1 = ctx->Input(1);
  xla::XlaBuilder* builder = ctx->builder();

  std::vector<std::vector<xla::XlaOp>> list_dynamic_dims;
  // Set dynamic dimension size to 0 for initialization value.
  std::vector<xla::XlaOp> dynamic_dims;
  dynamic_dims.push_back(leading_dim_is_dynamic 
                         ? input1 
                         : xla::ConstantR0<int32>(builder, num_elements));

  // Pre-compute all dynamic dimensions in one batch
  const int64_t num_dims = element_shape.dimensions_size();
  std::vector<xla::XlaOp> dynamic_dim_ops;
  for (int64_t dim = 0; dim < num_dims; ++dim) {
    if (dims_are_dynamic[dim]) {
      dynamic_dim_ops.push_back(xla::Reshape(
          xla::Slice(input0, {dim}, {dim + 1}, {1}), {}));
    }
  }
  if (!dynamic_dim_ops.empty()) {
    xla::XlaOp converted_dims = xla::ConvertElementType(
        xla::ConcatInDim(builder, dynamic_dim_ops, 0), xla::S32);
    auto dims_or = builder->GetShape(converted_dims);
    TF_RETURN_IF_ERROR(dims_or.status());
    const auto& dims_shape = dims_or.ValueOrDie();
    
    int dynamic_idx = 0;
    for (int64_t dim = 0; dim < num_dims; ++dim) {
      if (dims_are_dynamic[dim]) {
        dynamic_dims.push_back(xla::Slice(converted_dims, {dynamic_idx++}, 
                                         {dynamic_idx}, {1}));
      } else {
        dynamic_dims.push_back(
            xla::ConstantR0<int32>(builder, dynamic_sizes[dim]));
      }
    }
  } else {
    for (int64_t dim = 0; dim < num_dims; ++dim) {
      dynamic_dims.push_back(
          xla::ConstantR0<int32>(builder, dynamic_sizes[dim]));
    }
  }

  list_dynamic_dims.push_back(dynamic_dims);
  return list_dynamic_dims;
}