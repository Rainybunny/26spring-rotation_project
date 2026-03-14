LogicalResult matchAndRewrite(TF::InfeedDequeueTupleOp op,
                              PatternRewriter &rewriter) const override {
  // Check output types first
  auto outputs = op.outputs();
  const size_t num_outputs = outputs.size();
  std::vector<Type> result_types(num_outputs);
  
  for (size_t i = 0; i < num_outputs; ++i) {
    result_types[i] = outputs[i].getType();
    if (auto ty = result_types[i].dyn_cast<RankedTensorType>()) {
      if (!ty.hasStaticShape()) return failure();
    }
  }

  // Create token
  auto token = rewriter.create<CreateTokenOp>(
      op.getLoc(), mhlo::TokenType::get(rewriter.getContext()));

  // Create infeed op
  auto data_tuple_type = mlir::TupleType::get(rewriter.getContext(), result_types);
  auto data_and_token_type = mlir::TupleType::get(
      rewriter.getContext(), {data_tuple_type, token.getType()});

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
      *sharding_proto.add_tuple_shardings() = ::xla::sharding_builder::AssignDevice(0);
      data_and_token->setAttr(
          kShardingAttr,
          rewriter.getStringAttr(sharding_proto.SerializeAsString()));
    } else {
      data_and_token->setAttr(kShardingAttr, op._XlaShardingAttr());
    }
  }

  // Extract data tuple
  auto data_tuple = rewriter.create<GetTupleElementOp>(
      op.getLoc(), data_tuple_type, data_and_token,
      rewriter.getI32IntegerAttr(0));

  // Pre-allocate results
  SmallVector<Value, 4> results(num_outputs);
  for (size_t i = 0; i < num_outputs; ++i) {
    results[i] = rewriter.create<GetTupleElementOp>(
        op.getLoc(), result_types[i], data_tuple,
        rewriter.getI32IntegerAttr(i));
  }

  rewriter.replaceOp(op, results);
  return success();
}