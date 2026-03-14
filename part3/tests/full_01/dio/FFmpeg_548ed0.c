static void fill_from_cache(AVFilterContext *ctx, uint32_t *color, int *in_cidx, int *out_cidx, double py, double scale){
    MBContext *s = ctx->priv;
    if(s->morphamp)
        return;
    
    const double inv_scale = 1.0 / scale;
    const double x_offset = s->w/2 - s->start_x * inv_scale;
    
    for(; *in_cidx < s->cache_used; (*in_cidx)++){
        Point *p = &s->point_cache[*in_cidx];
        if(p->p[1] > py)
            break;
        int x = round(p->p[0] * inv_scale + x_offset);
        if((unsigned)x >= (unsigned)s->w)
            continue;
        if(color) color[x] = p->val;
        if(out_cidx && *out_cidx < s->cache_allocated)
            s->next_cache[(*out_cidx)++] = *p;
    }
}