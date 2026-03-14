static void fill_from_cache(AVFilterContext *ctx, uint32_t *color, int *in_cidx, int *out_cidx, double py, double scale) {
    MBContext *s = ctx->priv;
    if (s->morphamp)
        return;
    
    // Binary search to find first point with p->p[1] > py
    int left = *in_cidx;
    int right = s->cache_used - 1;
    int start = *in_cidx;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (s->point_cache[mid].p[1] <= py) {
            start = mid + 1;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    // Process from original in_cidx to start index
    for (; *in_cidx < start; (*in_cidx)++) {
        Point *p = &s->point_cache[*in_cidx];
        int x = round((p->p[0] - s->start_x) / scale + s->w/2);
        if (x >= 0 && x < s->w) {
            if (color) color[x] = p->val;
            if (out_cidx && *out_cidx < s->cache_allocated) {
                s->next_cache[(*out_cidx)++] = *p;
            }
        }
    }
}