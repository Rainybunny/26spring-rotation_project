LogicalResult matchAndRewrite(Operation *op,
                              PatternRewriter &rewriter) const override {
    if (op->getNumOperands() != 1) return failure();
    auto *defining_op = op->getOperand(0).getDefiningOp();
    auto defining_op_int =
        llvm::dyn_cast_or_null<InferShapedTypeOpInterface>(defining_op);
    if (!defining_op_int) return failure();
    SmallVector<ShapedTypeComponents, 4> components;
    if (failed(defining_op_int.inferReturnTypeComponents(
            op->getContext(), op->getLoc(), defining_op->getOperands(),
            defining_op->getAttrDictionary(), defining_op->getRegions(),
            components))) {
      return failure();
    }

    // Replace the op with another pass-through op with attributes added.
    OperationState state(op->getLoc(), "mhlo_test.return_type_components",
                         op->getOperands(), op->getResultTypes(),
                         op->getAttrs());
    auto *new_op = rewriter.createOperation(state);
    
    // Precompute attribute name prefixes
    const Twine dimsPrefix = "dims";
    const Twine elementTypePrefix = "element_type";
    
    for (auto it : llvm::enumerate(components)) {
      auto index = it.index();
      if (it.value().hasRank()) {
        new_op->setAttr((dimsPrefix + Twine(index)).str(),
                        rewriter.getI64ArrayAttr(it.value().getDims()));
      }
      if (it.value().getElementType()) {
        new_op->setAttr((elementTypePrefix + Twine(index)).str(),
                        TypeAttr::get(it.value().getElementType()));
      }
    }
    rewriter.replaceOp(op, {new_op->getResults()});
    return success();
  }