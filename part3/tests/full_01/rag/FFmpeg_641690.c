static av_cold int init(AVFilterContext *ctx)
{
    IDETContext *idet = ctx->priv;
    int i;

    idet->eof = 0;
    idet->last_type = UNDETERMINED;
    for (i = 0; i < HIST_SIZE; i++)
        idet->history[i] = UNDETERMINED;

    if( idet->half_life > 0 )
        idet->decay_coefficient = (uint64_t) round( PRECISION * exp2(-1.0 / idet->half_life) );
    else
        idet->decay_coefficient = PRECISION;

    idet->filter_line = ff_idet_filter_line_c;

    if (ARCH_X86)
        ff_idet_init_x86(idet, 0);

    return 0;
}