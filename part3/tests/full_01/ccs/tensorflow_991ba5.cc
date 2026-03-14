static SmallVector<AffineMap, 2> getIndexingMaps(OpTy op, Builder* b) {
  auto result_type = GetHloOpResultType(op).template cast<ShapedType>();
  auto nloops = result_type.getRank();
  
  // Pre-allocate and initialize input exprs in one pass
  SmallVector<AffineExpr, 2> input_exprs(nloops);
  auto permutation = op.permutation();
  for (int64_t i = 0; i < nloops; ++i) {
    input_exprs[permutation[i].getZExtValue()] = b->getAffineDimExpr(i);
  }

  // Reuse context instead of getting it repeatedly
  auto* context = b->getContext();
  return {
      AffineMap::get(nloops, /*symbolCount=*/0, input_exprs, context),
      b->getMultiDimIdentityMap(nloops)};
}