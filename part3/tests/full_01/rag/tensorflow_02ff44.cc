// Returns shape of a ranked tensor.
// Precondition: output_val's is ranked tensor.
DenseElementsAttr GetShape(Value output_val) {
  auto output_type = output_val.getType().cast<RankedTensorType>();
  auto shape = output_type.getShape();
  return mlir::DenseElementsAttr::get(
      RankedTensorType::get(
          {static_cast<int>(shape.size())},
          mlir::IntegerType::get(32, output_val.getContext())),
      llvm::ArrayRef<int64_t>(shape));
}