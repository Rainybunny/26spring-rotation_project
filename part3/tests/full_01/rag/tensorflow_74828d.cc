LogicalResult matchAndRewrite(mhlo::CstrReshapableOp op,
                              PatternRewriter &rewriter) const override {
  // Get shape analysis info for the number of elements.
  ShapeComponentAnalysis shapeComponentAnalysis;
  auto numElementsInfo = shapeComponentAnalysis.GetValueInfo(op.num_elements());
  if (!numElementsInfo) return failure();
  assert(numElementsInfo->size() == 1 && "expect one value for a scalar");
  auto numElements = numElementsInfo->front();

  // Get shape analysis info for the dynamic shape.
  auto dynShapeDims = shapeComponentAnalysis.GetValueInfo(op.dynamic_shape());
  if (!dynShapeDims) return failure();

  // We can only handle simple products with constants and symbols. Find all
  // the factors based on the number of elements.
  int64_t concreteProductNumElems = 1;
  SmallVector<Symbol> remainingSymbolicFactorsNumElems;
  if (!IsSimpleProduct(numElements, &concreteProductNumElems,
                       &remainingSymbolicFactorsNumElems)) {
    return failure();
  }
  assert(concreteProductNumElems >= 1 &&
         "number of elements cannot entail negative or zero factors");

  // Combined analysis of dynamic shape dimensions:
  // - Check for wildcard dimensions
  // - Accumulate concrete product
  // - Remove matching symbolic factors
  bool unique_wildcard_dimension = false;
  int64_t concreteProductDynShape = 1;
  for (auto dim : *dynShapeDims) {
    // Check for wildcard dimensions
    if (dim.isConstant(-1)) {
      if (unique_wildcard_dimension) return failure();
      unique_wildcard_dimension = true;
      continue;
    } else if (!dim.isKnownNotNegativeOne()) {
      return failure();
    }

    // Analyze factors
    SmallVector<Symbol> partialSymbolicFactorsDynShape;
    if (!IsSimpleProduct(
            dim,
            [&](int64_t c) { concreteProductDynShape *= c; },
            [&](Symbol s) { partialSymbolicFactorsDynShape.push_back(s); })) {
      return failure();
    }

    // Remove matching symbolic factors
    for (const Symbol &symDynShape : partialSymbolicFactorsDynShape) {
      auto *it = llvm::find(remainingSymbolicFactorsNumElems, symDynShape);
      if (it == remainingSymbolicFactorsNumElems.end()) return failure();
      remainingSymbolicFactorsNumElems.erase(it);
    }
  }
  assert(concreteProductDynShape >= 1 &&
         "concrete product must not aggregate negative or zero factors");

  // A wildcard dimension can subsume the remaining symbolic factors and
  // potentially also a concrete factor.
  if (unique_wildcard_dimension) {
    if (concreteProductNumElems % concreteProductDynShape != 0)
      return failure();
    rewriter.replaceOpWithNewOp<shape::ConstWitnessOp>(op, true);
    return success();
  }

  // W/o a wildcard, the symbolic and concrete products must be equal.
  bool isReshapable = remainingSymbolicFactorsNumElems.empty() &&
                      concreteProductNumElems == concreteProductDynShape;
  rewriter.replaceOpWithNewOp<shape::ConstWitnessOp>(op, isReshapable);
  return success();
}