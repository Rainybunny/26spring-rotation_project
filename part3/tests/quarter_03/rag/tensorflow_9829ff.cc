memref::ReinterpretCastOp InsertDynamicMemrefCastOp(
    mhlo::DynamicBroadcastInDimOp op, Value operand, OpBuilder* b) const {
  auto loc = op.getLoc();
  auto operand_type = operand.getType().cast<MemRefType>();
  auto operand_shape = operand_type.getShape();
  auto operand_rank = operand_type.getRank();

  auto result_type = op.getType().cast<RankedTensorType>();
  auto result_rank = result_type.getRank();

  // Precompute constants
  Value zero = b->create<arith::ConstantIndexOp>(loc, 0);
  Value one = b->create<arith::ConstantIndexOp>(loc, 1);

  // Compute strides and sizes
  SmallVector<Value, 2> operand_strides(operand_rank, one);
  SmallVector<Value, 2> operand_sizes(operand_rank, one);
  Value stride_so_far = one;
  
  // Optimized stride calculation loop
  for (int i = operand_rank - 1; i >= 0; --i) {
    Value operand_dim_size = ShapedType::isDynamic(operand_shape[i])
        ? b->create<memref::DimOp>(loc, operand, i)
        : b->create<arith::ConstantIndexOp>(loc, operand_shape[i]);
    
    operand_sizes[i] = operand_dim_size;
    operand_strides[i] = stride_so_far;
    stride_so_far = i > 0 ? b->create<arith::MulIOp>(loc, stride_so_far, operand_dim_size) : stride_so_far;
  }

  // Prepare output dimensions
  SmallVector<OpFoldResult, 2> sizes, strides;
  sizes.reserve(result_rank);
  strides.reserve(result_rank);

  // Build dimension mapping
  DenseMap<int, int> output_to_input_dim;
  for (auto dim : llvm::enumerate(op.broadcast_dimensions())) {
    output_to_input_dim[dim.value().getSExtValue()] = dim.index();
  }

  // Process each result dimension
  for (int i = 0; i < result_rank; ++i) {
    Value i_val = b->create<arith::ConstantIndexOp>(loc, i);
    Value result_dim_size = b->create<tensor::ExtractOp>(loc, op.output_dimensions(), i_val);
    if (!result_dim_size.getType().isIndex()) {
      result_dim_size = b->create<arith::IndexCastOp>(loc, result_dim_size, b->getIndexType());
    }
    
    sizes.push_back(result_type.isDynamicDim(i) ? result_dim_size : 
                   b->getIndexAttr(result_type.getDimSize(i)));

    auto it = output_to_input_dim.find(i);
    if (it == output_to_input_dim.end()) {
      strides.push_back(zero);
      continue;
    }

    int dim = it->second;
    Value cmp = b->create<arith::CmpIOp>(loc, arith::CmpIPredicate::slt, 
                                        operand_sizes[dim], result_dim_size);
    strides.push_back(b->create<mlir::SelectOp>(loc, cmp, zero, operand_strides[dim]));
  }

  // Create result type and cast
  SmallVector<int64_t, 2> dynamic_layout(result_rank, MemRefType::kDynamicStrideOrOffset);
  auto type_erased_memref_type = MemRefType::get(
      result_type.getShape(), operand_type.getElementType(),
      makeStridedLinearLayoutMap(dynamic_layout, 0, b->getContext()));

  return b->create<memref::ReinterpretCastOp>(
      loc, type_erased_memref_type, operand,
      b->getI64IntegerAttr(0), sizes, strides);
}