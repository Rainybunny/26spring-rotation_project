static SmallVector<AffineMap, 2> getIndexingMaps(OpTy op, Builder* b) {
  auto result_type = GetHloOpResultType(op).template cast<ShapedType>();
  auto nloops = result_type.getRank();
  
  // Directly construct the permutation map expressions
  SmallVector<AffineExpr, 4> input_exprs;
  input_exprs.reserve(nloops);
  for (int64_t i = 0; i < nloops; ++i) {
    input_exprs.push_back(b->getAffineDimExpr(i));
  }
  
  // Apply the permutation by swapping the expressions
  auto permutation = op.permutation();
  for (auto [i, perm_val] : llvm::enumerate(permutation)) {
    std::swap(input_exprs[i], input_exprs[perm_val.getZExtValue()]);
  }

  return {
      AffineMap::get(nloops, /*symbolCount=*/0, input_exprs, b->getContext()),
      b->getMultiDimIdentityMap(nloops)};
}