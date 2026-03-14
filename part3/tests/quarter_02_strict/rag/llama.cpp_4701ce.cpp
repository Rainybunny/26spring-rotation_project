static llama_token_data_array llama_sampling_prepare_impl(
                  struct llama_sampling_context * ctx_sampling,
                  struct llama_context * ctx_main,
                  struct llama_context * ctx_cfg,
                  const int idx,
                  bool apply_grammar,
                  std::vector<float> * original_logits) {
    const llama_sampling_params & params = ctx_sampling->params;

    const int n_vocab = llama_n_vocab(llama_get_model(ctx_main));
    const int32_t penalty_last_n  = params.penalty_last_n < 0 ? params.n_prev : params.penalty_last_n;
    const float   penalty_repeat  = params.penalty_repeat;
    const float   penalty_freq    = params.penalty_freq;
    const float   penalty_present = params.penalty_present;
    const bool    penalize_nl     = params.penalize_nl;
    const llama_token nl_token    = llama_token_nl(llama_get_model(ctx_main));

    auto & prev = ctx_sampling->prev;
    auto & cur  = ctx_sampling->cur;

    // Get logits pointer
    float * logits = llama_get_logits_ith(ctx_main, idx);

    if (ctx_sampling->grammar != NULL && !apply_grammar) {
        GGML_ASSERT(original_logits != NULL);
        *original_logits = {logits, logits + n_vocab};
    }

    // Apply guidance if needed
    if (ctx_cfg) {
        float * logits_guidance = llama_get_logits_ith(ctx_cfg, idx);
        llama_sample_apply_guidance(ctx_main, logits, logits_guidance, params.cfg_scale);
    }

    // Prepare token data array in one pass
    cur.resize(n_vocab);
    for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
        float logit = logits[token_id];
        // Apply logit bias if present
        auto it = params.logit_bias.find(token_id);
        if (it != params.logit_bias.end()) {
            logit += it->second;
        }
        cur[token_id] = {token_id, logit, 0.0f};
    }

    llama_token_data_array cur_p = { cur.data(), cur.size(), false };

    // Apply penalties if needed
    const auto& penalty_tokens = params.use_penalty_prompt_tokens ? params.penalty_prompt_tokens : prev;
    const int penalty_tokens_used_size = std::min((int)penalty_tokens.size(), penalty_last_n);
    if (penalty_tokens_used_size) {
        const float nl_logit = logits[nl_token];
        llama_sample_repetition_penalties(ctx_main, &cur_p,
                penalty_tokens.data() + penalty_tokens.size() - penalty_tokens_used_size,
                penalty_tokens_used_size, penalty_repeat, penalty_freq, penalty_present);

        if (!penalize_nl) {
            for (size_t idx = 0; idx < cur_p.size; idx++) {
                if (cur_p.data[idx].id == nl_token) {
                    cur_p.data[idx].logit = nl_logit;
                    break;
                }
            }
        }
    }

    // Apply grammar if needed
    if (apply_grammar && ctx_sampling->grammar != NULL) {
        llama_sample_grammar(ctx_main, &cur_p, ctx_sampling->grammar);
    }

    return cur_p;
}