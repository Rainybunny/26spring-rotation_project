LogicalResult MoveIntoAssumingOpMatchAndRewrite(Operation *op,
                                                PatternRewriter &rewriter) {
  // Find a preceding `assuming` op with nothing but side effect-free operations
  // in between.
  Operation *prev = op->getPrevNode();
  while (prev != nullptr && !llvm::isa<shape::AssumingOp>(prev) &&
         IsMovable(prev)) {
    prev = prev->getPrevNode();
  }
  auto assuming_op = llvm::dyn_cast_or_null<shape::AssumingOp>(prev);
  if (!assuming_op) return failure();

  Block *body = assuming_op.getBody();
  auto yield_op = cast<shape::AssumingYieldOp>(body->getTerminator());
  auto yield_operands = yield_op.getOperands();

  // Pre-allocate and process operands in single pass
  SmallVector<Value, 8> new_operands_unmapped;
  new_operands_unmapped.reserve(op->getNumOperands());
  
  for (Value v : op->getOperands()) {
    // Check availability while processing
    Operation *def = v.getDefiningOp();
    if (def && (def->getBlock() != op->getBlock() || 
                assuming_op->isBeforeInBlock(def))) {
      return failure();
    }

    // Map operands
    bool found = false;
    for (unsigned i = 0; i < assuming_op->getNumResults(); ++i) {
      if (assuming_op->getResult(i) == v) {
        new_operands_unmapped.push_back(yield_operands[i]);
        found = true;
        break;
      }
    }
    if (!found) new_operands_unmapped.push_back(v);
  }

  // Insert the rewritten assuming op right before the old one.
  OpBuilder::InsertionGuard guard(rewriter);
  rewriter.setInsertionPoint(assuming_op);
  auto new_assuming_op = rewriter.create<shape::AssumingOp>(
      assuming_op.getLoc(), assuming_op.getWitness(),
      [&](OpBuilder &b, Location) {
        // Copy body.
        BlockAndValueMapping mapping;
        for (auto &nested : body->without_terminator())
          b.clone(nested, mapping);

        // Copy op into the new body and use the mapped operands.
        for (auto it : llvm::zip(op->getOperands(), new_operands_unmapped)) {
          Value old_operand = std::get<0>(it);
          Value new_operand_unmapped = std::get<1>(it);
          mapping.map(old_operand, mapping.lookupOrDefault(new_operand_unmapped));
        }
        Operation *new_op = b.clone(*op, mapping);

        // Yield the previous results and also the new ones.
        auto mapped_results = llvm::to_vector<8>(llvm::map_range(
            yield_operands,
            [&](Value v) { return mapping.lookupOrDefault(v); }));
        mapped_results.append(new_op->getResults().begin(),
                            new_op->getResults().end());
        return mapped_results;
      });

  // Replace the assuming op and the root op with the corresponding result
  // value.
  ValueRange new_assuming_op_results = new_assuming_op->getResults();
  rewriter.replaceOp(assuming_op, new_assuming_op_results.drop_back());
  rewriter.replaceOp(op, new_assuming_op_results.back());
  return success();
}