LogicalResult matchAndRewrite(TF::InfeedDequeueTupleOp op,
                              PatternRewriter &rewriter) const override {
  // Verify all outputs have static shapes
  std::vector<Type> result_types(op.outputs().size());
  for (auto idx_and_output : llvm::enumerate(op.outputs())) {
    result_types[idx_and_output.index()] = idx_and_output.value().getType();
    if (auto ty = result_types.back().dyn_cast<RankedTensorType>()) {
      if (!ty.hasStaticShape()) return failure();
    }
  }

  // Create token for infeed
  auto token = rewriter.create<CreateTokenOp>(
      op.getLoc(), mhlo::TokenType::get(rewriter.getContext()));

  // Create infeed op with combined result type
  auto data_tuple_type = mlir::TupleType::get(rewriter.getContext(), result_types);
  auto data_and_token_type = mlir::TupleType::get(
      rewriter.getContext(), {data_tuple_type, token.getType()});

  // Create infeed operation
  auto data_and_token = rewriter.create<InfeedOp>(
      op.getLoc(), data_and_token_type, token,
      /*infeed_config=*/rewriter.getStringAttr(""),
      /*layout=*/ArrayAttr());

  // Handle sharding if present
  if (op._XlaSharding().hasValue()) {
    ::xla::OpSharding sharding_proto;
    if (!sharding_proto.ParseFromString(op._XlaSharding().getValue().str()))
      return failure();
    
    if (sharding_proto.type() == ::xla::OpSharding::TUPLE) {
      *sharding_proto.add_tuple_shardings() = 
          ::xla::sharding_builder::AssignDevice(0);
      data_and_token->setAttr(
          kShardingAttr,
          rewriter.getStringAttr(sharding_proto.SerializeAsString()));
    } else {
      data_and_token->setAttr(kShardingAttr, op._XlaShardingAttr());
    }
  }

  // Extract all tuple elements at once using a single operation
  SmallVector<Value> results;
  results.reserve(result_types.size());
  auto data_tuple = rewriter.create<GetTupleElementOp>(
      op.getLoc(), data_tuple_type, data_and_token,
      rewriter.getI32IntegerAttr(0));

  // Create one multi-result GetTupleElement instead of multiple single ones
  for (int i = 0; i < result_types.size(); ++i) {
    results.push_back(rewriter.create<GetTupleElementOp>(
        op.getLoc(), result_types[i], data_tuple,
        rewriter.getI32IntegerAttr(i)));
  }

  rewriter.replaceOp(op, results);
  return success();
}