static av_cold int init(AVFilterContext *ctx)
{
    IDETContext *idet = ctx->priv;

    idet->eof = 0;
    idet->last_type = UNDETERMINED;
    /* HIST_SIZE is small (5), so unroll the memset */
    idet->history[0] = idet->history[1] = idet->history[2] = 
    idet->history[3] = idet->history[4] = UNDETERMINED;

    if (ARCH_X86)
        ff_idet_init_x86(idet, 0);

    if (idet->half_life > 0) {
        /* Integer approximation of PRECISION * exp2(-1.0/half_life) */
        /* Using Taylor series approximation for exp2(x) ~= 1 + x*ln(2) when x is small */
        int64_t x = (-1LL << 20) / idet->half_life;  /* x = -1.0/half_life in fixed-point */
        idet->decay_coefficient = PRECISION + (x * 708LL / 1024LL); /* 708/1024 ~= ln(2) */
    } else {
        idet->decay_coefficient = PRECISION;
    }

    idet->filter_line = ff_idet_filter_line_c;

    return 0;
}