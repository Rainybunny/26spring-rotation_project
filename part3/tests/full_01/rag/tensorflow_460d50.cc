Value ProcessDotArg(Value arg, Location loc,
                    ArrayRef<int64_t> contract_dims_attr, bool outer_dims_first,
                    PatternRewriter *rewriter) {
  auto shape = arg.getType().cast<ShapedType>().getShape();
  const size_t rank = shape.size();

  // Pre-allocate vectors with known sizes
  llvm::SmallVector<bool, 5> is_outer_dim(rank, true);
  llvm::SmallVector<int64_t, 5> contract_dims;
  llvm::SmallVector<int64_t, 5> outer_dims;
  contract_dims.reserve(contract_dims_attr.size());
  outer_dims.reserve(rank - contract_dims_attr.size());

  // Single pass through dimensions
  for (int64_t dim : contract_dims_attr) {
    contract_dims.push_back(dim);
    is_outer_dim[dim] = false;
  }
  for (size_t i = 0; i < rank; ++i) {
    if (is_outer_dim[i]) {
      outer_dims.push_back(i);
    }
  }

  if (outer_dims_first) {
    return TransposeReshape(arg, loc, outer_dims, contract_dims, shape,
                            rewriter);
  }
  return TransposeReshape(arg, loc, contract_dims, outer_dims, shape, rewriter);
}