LogicalResult matchAndRewrite(mhlo::CstrReshapableOp op,
                              PatternRewriter &rewriter) const override {
  // Get shape analysis info for the number of elements.
  ShapeComponentAnalysis shapeComponentAnalysis;
  auto numElementsInfo = shapeComponentAnalysis.GetValueInfo(op.num_elements());
  if (!numElementsInfo || numElementsInfo->size() != 1) return failure();
  const auto &numElements = numElementsInfo->front();

  // Get shape analysis info for the dynamic shape.
  auto dynShapeDims = shapeComponentAnalysis.GetValueInfo(op.dynamic_shape());
  if (!dynShapeDims) return failure();

  // First pass: check for wildcard dimensions and collect concrete products
  bool unique_wildcard_dimension = false;
  int64_t concreteProductDynShape = 1;
  SmallVector<Symbol> allSymbolicFactorsDynShape;
  
  for (const auto &dim : *dynShapeDims) {
    if (dim.isConstant(-1)) {
      if (unique_wildcard_dimension) return failure();
      unique_wildcard_dimension = true;
      continue;
    } else if (!dim.isKnownNotNegativeOne()) {
      return failure();
    }

    SmallVector<Symbol> partialSymbolicFactors;
    if (!IsSimpleProduct(
            dim,
            [&](int64_t c) { concreteProductDynShape *= c; },
            [&](Symbol s) { partialSymbolicFactors.push_back(s); })) {
      return failure();
    }
    allSymbolicFactorsDynShape.append(partialSymbolicFactors);
  }

  // Find all factors based on the number of elements.
  int64_t concreteProductNumElems = 1;
  SmallVector<Symbol> remainingSymbolicFactorsNumElems;
  if (!IsSimpleProduct(numElements, &concreteProductNumElems,
                       &remainingSymbolicFactorsNumElems)) {
    return failure();
  }

  // Remove symbolic factors from dynamic shape
  for (const Symbol &symDynShape : allSymbolicFactorsDynShape) {
    auto *it = llvm::find(remainingSymbolicFactorsNumElems, symDynShape);
    if (it == remainingSymbolicFactorsNumElems.end()) return failure();
    remainingSymbolicFactorsNumElems.erase(it);
  }

  // A wildcard dimension can subsume remaining factors
  if (unique_wildcard_dimension) {
    if (concreteProductNumElems % concreteProductDynShape != 0)
      return failure();
    rewriter.replaceOpWithNewOp<shape::ConstWitnessOp>(op, true);
    return success();
  }

  // W/o a wildcard, products must be equal
  bool isReshapable = remainingSymbolicFactorsNumElems.empty() &&
                      concreteProductNumElems == concreteProductDynShape;
  rewriter.replaceOpWithNewOp<shape::ConstWitnessOp>(op, isReshapable);
  return success();
}