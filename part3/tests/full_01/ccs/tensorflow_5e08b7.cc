LogicalResult matchAndRewrite(Operation *op,
                              PatternRewriter &rewriter) const override {
    if (op->getNumOperands() != 1) return failure();
    Value operand = op->getOperand(0);
    auto *defining_op = operand.getDefiningOp();
    auto defining_op_int =
        llvm::dyn_cast_or_null<InferShapedTypeOpInterface>(defining_op);
    if (!defining_op_int) return failure();

    // Cache frequently used values from defining_op
    MLIRContext *context = op->getContext();
    Location loc = op->getLoc();
    auto defining_operands = defining_op->getOperands();
    auto defining_attrs = defining_op->getAttrDictionary();
    auto defining_regions = defining_op->getRegions();

    SmallVector<ShapedTypeComponents, 4> components;
    if (failed(defining_op_int.inferReturnTypeComponents(
            context, loc, defining_operands, defining_attrs, defining_regions,
            components))) {
      return failure();
    }

    // Replace the op with another pass-through op with attributes added.
    OperationState state(loc, "mhlo_test.return_type_components",
                         op->getOperands(), op->getResultTypes(),
                         op->getAttrs());
    auto *new_op = rewriter.createOperation(state);
    for (auto it : llvm::enumerate(components)) {
      const auto &component = it.value();
      if (component.hasRank()) {
        new_op->setAttr((StringRef("dims") + Twine(it.index())).str(),
                        rewriter.getI64ArrayAttr(component.getDims()));
      }
      if (component.getElementType()) {
        new_op->setAttr((Twine("element_type") + Twine(it.index())).str(),
                        TypeAttr::get(component.getElementType()));
      }
    }
    rewriter.replaceOp(op, {new_op->getResults()});
    return success();
  }