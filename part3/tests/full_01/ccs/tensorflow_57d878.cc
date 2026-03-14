StatusOr<std::vector<std::vector<xla::XlaOp>>> GetTensorListDynamicDims(
    XlaOpKernelContext* ctx, const xla::Shape& element_shape,
    const xla::Shape& list_shape, int64_t num_elements) {
  // Cache frequently used values
  xla::XlaOp input0 = ctx->Input(0);
  xla::XlaOp input1 = ctx->Input(1);
  xla::XlaBuilder* builder = ctx->builder();
  
  std::vector<int64_t> dynamic_sizes;
  TF_RETURN_IF_ERROR(ctx->ConstantInputAsIntVector(0, &dynamic_sizes));
  std::vector<bool> dims_are_dynamic;
  TF_RETURN_IF_ERROR(
      ctx->ResolveInputDynamismIntoPredVector(0, &dims_are_dynamic));
  bool leading_dim_is_dynamic;
  TF_RETURN_IF_ERROR(
      ctx->ResolveInputDynamismIntoPred(1, &leading_dim_is_dynamic));
  
  std::vector<std::vector<xla::XlaOp>> list_dynamic_dims;
  std::vector<xla::XlaOp> dynamic_dims;
  
  // Cache dimensions size
  const int64_t dims_size = element_shape.dimensions_size();
  
  if (leading_dim_is_dynamic) {
    dynamic_dims.push_back(input1);
  } else {
    dynamic_dims.push_back(xla::ConstantR0<int32>(builder, num_elements));
  }
  
  for (int64_t dim = 0; dim < dims_size; ++dim) {
    if (dims_are_dynamic[dim]) {
      auto dynamic_dim_size = xla::Slice(input0, {dim}, {dim + 1}, {1});
      dynamic_dim_size = xla::Reshape(dynamic_dim_size, {});
      dynamic_dim_size = xla::ConvertElementType(dynamic_dim_size, xla::S32);
      dynamic_dims.push_back(dynamic_dim_size);
    } else {
      dynamic_dims.push_back(
          xla::ConstantR0<int32>(builder, dynamic_sizes[dim]));
    }
  }
  
  list_dynamic_dims.push_back(dynamic_dims);
  return list_dynamic_dims;
}