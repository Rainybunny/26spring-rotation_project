void MarkShapeCalc::markI64ReturnedCpuScalarOps(
    FuncOp func, llvm::DenseSet<Operation*>& shape_calc_ops) {
  assert(func.getName() == "main");
  auto* return_op = func.front().getTerminator();
  if (!isa<mlir::ReturnOp>(return_op)) return;
  auto result_attrs = func.getAllResultAttrs();
  if (!result_attrs) return;
  auto returned_ops = return_op->getOperands();
  assert(returned_ops.size() == result_attrs.size());

  // Cache placement comparison outside loop
  const auto& output_placement_key = *output_placement_attr_key_;
  const auto& cpu_attr = cpu_placement_attr_;

  for (auto output : llvm::enumerate(returned_ops)) {
    Operation* op = output.value().getDefiningOp();
    if (!op || !isMhloDialect(op)) continue;
    
    auto result = op->getResult(0);
    auto type = result.getType().dyn_cast<RankedTensorType>();
    if (!type || !type.getElementType().isInteger(64) || type.getRank() != 0) {
      continue;
    }

    // Cache dictionary attr lookup
    auto dict_attr = result_attrs[output.index()].cast<DictionaryAttr>();
    if (dict_attr.getAs<StringAttr>(output_placement_key) == cpu_attr) {
      shape_calc_ops.insert(op);
    }
  }
}