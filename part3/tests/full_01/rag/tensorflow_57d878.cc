StatusOr<std::vector<std::vector<xla::XlaOp>>> GetTensorListDynamicDims(
    XlaOpKernelContext* ctx, const xla::Shape& element_shape,
    const xla::Shape& list_shape, int64_t num_elements) {
  std::vector<int64_t> dynamic_sizes;
  TF_RETURN_IF_ERROR(ctx->ConstantInputAsIntVector(0, &dynamic_sizes));
  
  std::vector<bool> dims_are_dynamic;
  TF_RETURN_IF_ERROR(
      ctx->ResolveInputDynamismIntoPredVector(0, &dims_are_dynamic));
  
  bool leading_dim_is_dynamic;
  TF_RETURN_IF_ERROR(
      ctx->ResolveInputDynamismIntoPred(1, &leading_dim_is_dynamic));

  const int64_t num_dims = element_shape.dimensions_size();
  std::vector<std::vector<xla::XlaOp>> list_dynamic_dims;
  std::vector<xla::XlaOp> dynamic_dims;
  dynamic_dims.reserve(num_dims + 1);  // Pre-allocate space

  // Handle leading dimension
  dynamic_dims.push_back(leading_dim_is_dynamic
                         ? ctx->Input(1)
                         : xla::ConstantR0<int32>(ctx->builder(), num_elements));

  // Process remaining dimensions
  xla::XlaOp input0 = ctx->Input(0);
  xla::XlaBuilder* builder = ctx->builder();
  for (int64_t dim = 0; dim < num_dims; ++dim) {
    if (dims_are_dynamic[dim]) {
      xla::XlaOp dynamic_dim_size = xla::Slice(input0, {dim}, {dim + 1}, {1});
      dynamic_dim_size = xla::Reshape(dynamic_dim_size, {});
      dynamic_dims.push_back(xla::ConvertElementType(dynamic_dim_size, xla::S32));
    } else {
      dynamic_dims.push_back(xla::ConstantR0<int32>(builder, dynamic_sizes[dim]));
    }
  }

  list_dynamic_dims.push_back(std::move(dynamic_dims));
  return list_dynamic_dims;
}