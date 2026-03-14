// Returns shape of a ranked tensor.
// Precondition: output_val's is ranked tensor.
DenseElementsAttr GetShape(Value output_val) {
  auto output_type = output_val.getType().cast<RankedTensorType>();
  auto shape_vector = output_type.getShape();
  llvm::SmallVector<int32_t, 4> shape(shape_vector.begin(), shape_vector.end());
  return mlir::DenseElementsAttr::get(
      RankedTensorType::get(
          {static_cast<int>(shape.size())},
          mlir::IntegerType::get(32, output_val.getContext())),
      llvm::makeArrayRef(shape));
}