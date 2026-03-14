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
  std::vector<std::vector<xla::XlaOp>> list_dynamic_dims;
  std::vector<xla::XlaOp> dynamic_dims;
  
  // Handle leading dimension
  if (leading_dim_is_dynamic) {
    dynamic_dims.push_back(ctx->Input(1));
  } else {
    dynamic_dims.push_back(
        xla::ConstantR0<int32>(ctx->builder(), num_elements));
  }

  // Batch process all dynamic dimensions
  std::vector<int64_t> dynamic_indices;
  for (int64_t dim = 0; dim < element_shape.dimensions_size(); ++dim) {
    if (dims_are_dynamic[dim]) {
      dynamic_indices.push_back(dim);
    }
  }

  if (!dynamic_indices.empty()) {
    // Slice all dynamic dimensions at once
    auto dynamic_dim_sizes = xla::Slice(
        ctx->Input(0), dynamic_indices,
        std::vector<int64_t>(dynamic_indices.size(), 1),
        std::vector<int64_t>(dynamic_indices.size(), 1));
    
    // Reshape and convert all at once
    dynamic_dim_sizes = xla::Reshape(dynamic_dim_sizes, 
                                    {static_cast<int64_t>(dynamic_indices.size())});
    dynamic_dim_sizes = xla::ConvertElementType(dynamic_dim_sizes, xla::S32);
    
    // Distribute results to their positions
    int dynamic_pos = 0;
    for (int64_t dim = 0; dim < element_shape.dimensions_size(); ++dim) {
      if (dims_are_dynamic[dim]) {
        dynamic_dims.push_back(xla::SliceInDim(dynamic_dim_sizes, dynamic_pos, 
                                              dynamic_pos + 1, 1, 0));
        dynamic_pos++;
      } else {
        dynamic_dims.push_back(
            xla::ConstantR0<int32>(ctx->builder(), dynamic_sizes[dim]));
      }
    }
  } else {
    // All dimensions are static
    for (int64_t dim = 0; dim < element_shape.dimensions_size(); ++dim) {
      dynamic_dims.push_back(
          xla::ConstantR0<int32>(ctx->builder(), dynamic_sizes[dim]));
    }
  }

  list_dynamic_dims.push_back(dynamic_dims);
  return list_dynamic_dims;
}