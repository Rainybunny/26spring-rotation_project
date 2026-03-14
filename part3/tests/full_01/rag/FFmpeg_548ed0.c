static void fill_from_cache(AVFilterContext *ctx, uint32_t *color, int *in_cidx, int *out_cidx, double py, double scale){
    MBContext *s = ctx->priv;
    static int last_idx = 0; // Static variable to maintain state between calls
    if(s->morphamp)
        return;
    
    // Start from last position or provided index, whichever is larger
    int start_idx = FFMAX(last_idx, *in_cidx);
    for(int i = start_idx; i < s->cache_used; i++){
        Point *p= &s->point_cache[i];
        if(p->p[1] > py) {
            last_idx = i; // Remember where we stopped
            *in_cidx = i; // Update caller's index
            break;
        }
        int x= round((p->p[0] - s->start_x) / scale + s->w/2);
        if((unsigned)x >= (unsigned)s->w)
            continue;
        if(color) color[x] = p->val;
        if(out_cidx && *out_cidx < s->cache_allocated)
            s->next_cache[(*out_cidx)++]= *p;
    }
    // If we reached the end, update both indices
    if(*in_cidx >= s->cache_used) {
        last_idx = s->cache_used;
        *in_cidx = s->cache_used;
    }
}