Value ProcessDotArg(Value arg, Location loc,
                    ArrayRef<int64_t> contract_dims_attr, bool outer_dims_first,
                    PatternRewriter *rewriter) {
  auto shape = arg.getType().cast<ShapedType>().getShape();
  const size_t rank = shape.size();

  // Pre-allocate vectors with exact sizes
  llvm::SmallVector<int64_t, 5> contract_dims;
  llvm::SmallVector<int64_t, 5> outer_dims;
  contract_dims.reserve(contract_dims_attr.size());
  outer_dims.reserve(rank - contract_dims_attr.size());

  // Process all dimensions in a single pass
  for (int64_t dim = 0; dim < rank; ++dim) {
    if (llvm::is_contained(contract_dims_attr, dim)) {
      contract_dims.push_back(dim);
    } else {
      outer_dims.push_back(dim);
    }
  }

  if (outer_dims_first) {
    return TransposeReshape(arg, loc, outer_dims, contract_dims, shape,
                            rewriter);
  }

  return TransposeReshape(arg, loc, contract_dims, outer_dims, shape, rewriter);
}