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
    case 1: {
        double coeffs[4] = {0};
        int imins[4] = {0}, imaxs[4] = {0}, omins[4] = {0}, omaxs[4] = {0};
        
        // Precompute all coefficients first
        for (i = 0; i < s->nb_comp; i++) {
            Range *r = &s->range[i];
            const uint8_t offset = s->rgba_map[i];
            const uint8_t *srcrow = in->data[0];
            
            imins[i] = round(r->in_min  * UINT8_MAX);
            imaxs[i] = round(r->in_max  * UINT8_MAX);
            omins[i] = round(r->out_min * UINT8_MAX);
            omaxs[i] = round(r->out_max * UINT8_MAX);

            if (imins[i] < 0) {
                imins[i] = UINT8_MAX;
                for (y = 0; y < inlink->h; y++) {
                    const uint8_t *src = srcrow;
                    for (x = 0; x < s->linesize; x += step)
                        imins[i] = FFMIN(imins[i], src[x + offset]);
                    srcrow += in->linesize[0];
                }
            }
            if (imaxs[i] < 0) {
                srcrow = in->data[0];
                imaxs[i] = 0;
                for (y = 0; y < inlink->h; y++) {
                    const uint8_t *src = srcrow;
                    for (x = 0; x < s->linesize; x += step)
                        imaxs[i] = FFMAX(imaxs[i], src[x + offset]);
                    srcrow += in->linesize[0];
                }
            }
            coeffs[i] = (omaxs[i] - omins[i]) / (double)(imaxs[i] - imins[i]);
        }

        // Process all pixels with precomputed coefficients
        for (y = 0; y < inlink->h; y++) {
            const uint8_t *src = in->data[0] + y * in->linesize[0];
            uint8_t *dst = out->data[0] + y * out->linesize[0];
            
            for (x = 0; x < s->linesize; x += step) {
                for (i = 0; i < s->nb_comp; i++) {
                    const uint8_t offset = s->rgba_map[i];
                    dst[x + offset] = av_clip_uint8((src[x + offset] - imins[i]) * coeffs[i] + omins[i]);
                }
            }
        }
        break;
    }
    case 2: {
        double coeffs[4] = {0};
        int imins[4] = {0}, imaxs[4] = {0}, omins[4] = {0}, omaxs[4] = {0};
        
        // Precompute all coefficients first
        for (i = 0; i < s->nb_comp; i++) {
            Range *r = &s->range[i];
            const uint8_t offset = s->rgba_map[i];
            const uint8_t *srcrow = in->data[0];
            
            imins[i] = round(r->in_min  * UINT16_MAX);
            imaxs[i] = round(r->in_max  * UINT16_MAX);
            omins[i] = round(r->out_min * UINT16_MAX);
            omaxs[i] = round(r->out_max * UINT16_MAX);

            if (imins[i] < 0) {
                imins[i] = UINT16_MAX;
                for (y = 0; y < inlink->h; y++) {
                    const uint16_t *src = (const uint16_t *)srcrow;
                    for (x = 0; x < s->linesize; x += step)
                        imins[i] = FFMIN(imins[i], src[x + offset]);
                    srcrow += in->linesize[0];
                }
            }
            if (imaxs[i] < 0) {
                srcrow = in->data[0];
                imaxs[i] = 0;
                for (y = 0; y < inlink->h; y++) {
                    const uint16_t *src = (const uint16_t *)srcrow;
                    for (x = 0; x < s->linesize; x += step)
                        imaxs[i] = FFMAX(imaxs[i], src[x + offset]);
                    srcrow += in->linesize[0];
                }
            }
            coeffs[i] = (omaxs[i] - omins[i]) / (double)(imaxs[i] - imins[i]);
        }

        // Process all pixels with precomputed coefficients
        for (y = 0; y < inlink->h; y++) {
            const uint16_t *src = (const uint16_t *)(in->data[0] + y * in->linesize[0]);
            uint16_t *dst = (uint16_t *)(out->data[0] + y * out->linesize[0]);
            
            for (x = 0; x < s->linesize; x += step) {
                for (i = 0; i < s->nb_comp; i++) {
                    const uint8_t offset = s->rgba_map[i];
                    dst[x + offset] = av_clip_uint16((src[x + offset] - imins[i]) * coeffs[i] + omins[i]);
                }
            }
        }
        break;
    }
    }

    if (in != out)
        av_frame_free(&in);
    return ff_filter_frame(outlink, out);
}