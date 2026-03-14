static SmallVector<AffineMap, 2> getIndexingMaps(OpTy op, Builder* b) {
  auto result_type = GetHloOpResultType(op).template cast<ShapedType>();
  auto nloops = result_type.getRank();
  
  // Pre-allocate with identity map expressions
  SmallVector<AffineExpr, 4> input_exprs;
  for (int i = 0; i < nloops; ++i) {
    input_exprs.push_back(b->getAffineDimExpr(i));
  }

  // Apply permutation by swapping the expressions
  auto permutation = op.permutation();
  for (const auto& [idx, perm_val] : llvm::enumerate(permutation)) {
    std::swap(input_exprs[idx], input_exprs[perm_val.getZExtValue()]);
  }

  return {
      AffineMap::get(nloops, /*symbolCount=*/0, input_exprs, b->getContext()),
      b->getMultiDimIdentityMap(nloops)};
}