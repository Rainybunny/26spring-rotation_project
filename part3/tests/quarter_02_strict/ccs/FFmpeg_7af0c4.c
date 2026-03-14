static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    AVFilterContext *ctx = inlink->dst;
    ColorLevelsContext *s = ctx->priv;
    AVFilterLink *outlink = ctx->outputs[0];
    const int step = s->step;
    AVFrame *out;
    int x, y, i;

    if (av_frame_is_writable(in)) {
        out = in;
    } else {
        out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
        if (!out) {
            av_frame_free(&in);
            return AVERROR(ENOMEM);
        }
        av_frame_copy_props(out, in);
    }

    switch (s->bpp) {
    case 1:
        for (i = 0; i < s->nb_comp; i++) {
            Range *r = &s->range[i];
            const uint8_t offset = s->rgba_map[i];
            const uint8_t *src = in->data[0];
            uint8_t *dst = out->data[0];
            int imin = round(r->in_min  * UINT8_MAX);
            int imax = round(r->in_max  * UINT8_MAX);
            int omin = round(r->out_min * UINT8_MAX);
            int omax = round(r->out_max * UINT8_MAX);
            double coeff;
            int need_min = (imin < 0);
            int need_max = (imax < 0);

            if (need_min) imin = UINT8_MAX;
            if (need_max) imax = 0;

            for (y = 0; y < inlink->h; y++) {
                const uint8_t *src_p = src;
                uint8_t *dst_p = dst;

                for (x = 0; x < s->linesize; x += step) {
                    int val = src_p[x + offset];
                    if (need_min) imin = FFMIN(imin, val);
                    if (need_max) imax = FFMAX(imax, val);
                    dst_p[x + offset] = val;
                }
                src += in->linesize[0];
                dst += out->linesize[0];
            }

            coeff = (omax - omin) / (double)(imax - imin);
            src = in->data[0];
            dst = out->data[0];

            for (y = 0; y < inlink->h; y++) {
                const uint8_t *src_p = src;
                uint8_t *dst_p = dst;

                for (x = 0; x < s->linesize; x += step) {
                    dst_p[x + offset] = av_clip_uint8((src_p[x + offset] - imin) * coeff + omin);
                }
                src += in->linesize[0];
                dst += out->linesize[0];
            }
        }
        break;
    case 2:
        for (i = 0; i < s->nb_comp; i++) {
            Range *r = &s->range[i];
            const uint8_t offset = s->rgba_map[i];
            const uint8_t *src = in->data[0];
            uint8_t *dst = out->data[0];
            int imin = round(r->in_min  * UINT16_MAX);
            int imax = round(r->in_max  * UINT16_MAX);
            int omin = round(r->out_min * UINT16_MAX);
            int omax = round(r->out_max * UINT16_MAX);
            double coeff;
            int need_min = (imin < 0);
            int need_max = (imax < 0);

            if (need_min) imin = UINT16_MAX;
            if (need_max) imax = 0;

            for (y = 0; y < inlink->h; y++) {
                const uint16_t *src_p = (const uint16_t *)src;
                uint16_t *dst_p = (uint16_t *)dst;

                for (x = 0; x < s->linesize; x += step) {
                    int val = src_p[x + offset];
                    if (need_min) imin = FFMIN(imin, val);
                    if (need_max) imax = FFMAX(imax, val);
                    dst_p[x + offset] = val;
                }
                src += in->linesize[0];
                dst += out->linesize[0];
            }

            coeff = (omax - omin) / (double)(imax - imin);
            src = in->data[0];
            dst = out->data[0];

            for (y = 0; y < inlink->h; y++) {
                const uint16_t *src_p = (const uint16_t *)src;
                uint16_t *dst_p = (uint16_t *)dst;

                for (x = 0; x < s->linesize; x += step) {
                    dst_p[x + offset] = av_clip_uint16((src_p[x + offset] - imin) * coeff + omin);
                }
                src += in->linesize[0];
                dst += out->linesize[0];
            }
        }
    }

    if (in != out)
        av_frame_free(&in);
    return ff_filter_frame(outlink, out);
}