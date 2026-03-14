explicit TtlMergeOperator(const std::shared_ptr<MergeOperator> merge_op,
                            Env* env) noexcept
      : user_merge_op_(merge_op), env_(env) {
    assert(merge_op);
    assert(env);
  }