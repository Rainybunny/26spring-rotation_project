static void
fz_insert_gel_raw(fz_gel *gel, int x0, int y0, int x1, int y1)
{
    fz_edge *edge;
    int dx, dy;
    int width;
    int tmp;

    if (y0 == y1)
        return;

    // Branchless winding calculation and swap
    int swap = y0 > y1;
    winding = 1 - (swap << 1); // Results in 1 or -1
    tmp = x0; x0 = (swap ? x1 : x0); x1 = (swap ? tmp : x1);
    tmp = y0; y0 = (swap ? y1 : y0); y1 = (swap ? tmp : y1);

    // Branchless bbox updates
    gel->bbox.x0 = fz_mini(gel->bbox.x0, fz_mini(x0, x1));
    gel->bbox.x1 = fz_maxi(gel->bbox.x1, fz_maxi(x0, x1));
    gel->bbox.y0 = fz_mini(gel->bbox.y0, y0);
    gel->bbox.y1 = fz_maxi(gel->bbox.y1, y1);

    if (gel->len + 1 == gel->cap) {
        int new_cap = gel->cap + 512;
        gel->edges = fz_resize_array(gel->ctx, gel->edges, new_cap, sizeof(fz_edge));
        gel->cap = new_cap;
    }

    edge = &gel->edges[gel->len++];

    dy = y1 - y0;
    dx = x1 - x0;
    width = fz_absi(dx);

    edge->xdir = (dx > 0) ? 1 : -1;
    edge->ydir = winding;
    edge->x = x0;
    edge->y = y0;
    edge->h = dy;
    edge->adj_down = dy;

    // Branchless error term calculation
    edge->e = (dx >= 0) ? 0 : (-dy + 1);

    // Branchless edge type selection
    int y_major = dy >= width;
    edge->xmove = y_major ? 0 : ((width / dy) * edge->xdir);
    edge->adj_up = y_major ? width : (width % dy);
}