class ConvertInfeedDequeueTupleOp
    : public OpRewritePattern<TF::InfeedDequeueTupleOp> {
 public:
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(TF::InfeedDequeueTupleOp op,
                                PatternRewriter &rewriter) const override {
    // Collect all output types in a vector
    SmallVector<Type, 4> result_types;
    result_types.reserve(op.outputs().size());
    for (const auto &output : op.outputs()) {
      auto ty = output.getType().dyn_cast<RankedTensorType>();
      if (!ty || !ty.hasStaticShape()) return failure();
      result_types.push_back(output.getType());
    }

    // Create token once
    auto token = rewriter.create<CreateTokenOp>(
        op.getLoc(), mhlo::TokenType::get(rewriter.getContext()));

    // Create combined infeed op
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
      if (!sharding_proto.ParseFromString(op._XlaSharding()->str())) {
        return failure();
      }
      
      if (sharding_proto.type() == ::xla::OpSharding::TUPLE) {
        *sharding_proto.add_tuple_shardings() = 
            ::xla::sharding_builder::AssignDevice(0);
      }
      data_and_token->setAttr(
          kShardingAttr,
          rewriter.getStringAttr(sharding_proto.SerializeAsString()));
    }

    // Get all tuple elements at once
    auto data_tuple = rewriter.create<GetTupleElementOp>(
        op.getLoc(), data_tuple_type, data_and_token,
        rewriter.getI32IntegerAttr(0));

    // Create all result elements in one batch
    SmallVector<Value, 4> results;
    results.reserve(result_types.size());
    for (auto idx : llvm::seq<size_t>(0, result_types.size())) {
      results.push_back(rewriter.create<GetTupleElementOp>(
          op.getLoc(), result_types[idx], data_tuple,
          rewriter.getI32IntegerAttr(idx)));
    }

    rewriter.replaceOp(op, results);
    return success();
  }
};