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
            const uint8_t *srcrow = in->data[0];
            uint8_t *dstrow = out->data[0];
            int imin = round(r->in_min  * UINT8_MAX);
            int imax = round(r->in_max  * UINT8_MAX);
            int omin = round(r->out_min * UINT8_MAX);
            int omax = round(r->out_max * UINT8_MAX);
            double coeff;

            if (imin < 0) {
#ifdef __SSE2__
                __m128i min_vals = _mm_set1_epi8(0xFF);
                for (y = 0; y < inlink->h; y++) {
                    const uint8_t *src = srcrow;
                    int x_end = s->linesize - (s->linesize % 16);
                    
                    for (x = offset; x < x_end; x += step * 16) {
                        __m128i vals = _mm_loadu_si128((const __m128i*)(src + x));
                        min_vals = _mm_min_epu8(min_vals, vals);
                    }
                    // Handle remaining pixels
                    for (; x < s->linesize; x += step)
                        imin = FFMIN(imin, src[x]);
                    
                    srcrow += in->linesize[0];
                }
                // Find min across SIMD register
                uint8_t min_buf[16];
                _mm_storeu_si128((__m128i*)min_buf, min_vals);
                for (int j = 0; j < 16; j++)
                    imin = FFMIN(imin, min_buf[j]);
#else
                imin = UINT8_MAX;
                for (y = 0; y < inlink->h; y++) {
                    const uint8_t *src = srcrow;
                    for (x = 0; x < s->linesize; x += step)
                        imin = FFMIN(imin, src[x + offset]);
                    srcrow += in->linesize[0];
                }
#endif
            }
            if (imax < 0) {
                srcrow = in->data[0];
#ifdef __SSE2__
                __m128i max_vals = _mm_setzero_si128();
                for (y = 0; y < inlink->h; y++) {
                    const uint8_t *src = srcrow;
                    int x_end = s->linesize - (s->linesize % 16);
                    
                    for (x = offset; x < x_end; x += step * 16) {
                        __m128i vals = _mm_loadu_si128((const __m128i*)(src + x));
                        max_vals = _mm_max_epu8(max_vals, vals);
                    }
                    // Handle remaining pixels
                    for (; x < s->linesize; x += step)
                        imax = FFMAX(imax, src[x]);
                    
                    srcrow += in->linesize[0];
                }
                // Find max across SIMD register
                uint8_t max_buf[16];
                _mm_storeu_si128((__m128i*)max_buf, max_vals);
                for (int j = 0; j < 16; j++)
                    imax = FFMAX(imax, max_buf[j]);
#else
                imax = 0;
                for (y = 0; y < inlink->h; y++) {
                    const uint8_t *src = srcrow;
                    for (x = 0; x < s->linesize; x += step)
                        imax = FFMAX(imax, src[x + offset]);
                    srcrow += in->linesize[0];
                }
#endif
            }

            srcrow = in->data[0];
            coeff = (omax - omin) / (double)(imax - imin);
#ifdef __SSE2__
            __m128i v_imin = _mm_set1_epi16(imin);
            __m128i v_omin = _mm_set1_epi16(omin);
            __m128i v_coeff = _mm_set1_ps(coeff);
            
            for (y = 0; y < inlink->h; y++) {
                const uint8_t *src = srcrow;
                uint8_t *dst = dstrow;
                int x_end = s->linesize - (s->linesize % 16);
                
                for (x = offset; x < x_end; x += step * 16) {
                    __m128i in_pixels = _mm_loadu_si128((const __m128i*)(src + x));
                    // Process 16 pixels at once
                    __m128i result = _mm_subs_epu8(in_pixels, v_imin);
                    result = _mm_mulhi_epu16(result, v_coeff);
                    result = _mm_adds_epu8(result, v_omin);
                    _mm_storeu_si128((__m128i*)(dst + x), result);
                }
                // Handle remaining pixels
                for (; x < s->linesize; x += step)
                    dst[x + offset] = av_clip_uint8((src[x + offset] - imin) * coeff + omin);
                
                dstrow += out->linesize[0];
                srcrow += in->linesize[0];
            }
#else
            for (y = 0; y < inlink->h; y++) {
                const uint8_t *src = srcrow;
                uint8_t *dst = dstrow;
                for (x = 0; x < s->linesize; x += step)
                    dst[x + offset] = av_clip_uint8((src[x + offset] - imin) * coeff + omin);
                dstrow += out->linesize[0];
                srcrow += in->linesize[0];
            }
#endif
        }
        break;
    case 2:
        // Keep original 16-bit implementation
        for (i = 0; i < s->nb_comp; i++) {
            Range *r = &s->range[i];
            const uint8_t offset = s->rgba_map[i];
            const uint8_t *srcrow = in->data[0];
            uint8_t *dstrow = out->data[0];
            int imin = round(r->in_min  * UINT16_MAX);
            int imax = round(r->in_max  * UINT16_MAX);
            int omin = round(r->out_min * UINT16_MAX);
            int omax = round(r->out_max * UINT16_MAX);
            double coeff;

            if (imin < 0) {
                imin = UINT16_MAX;
                for (y = 0; y < inlink->h; y++) {
                    const uint16_t *src = (const uint16_t *)srcrow;
                    for (x = 0; x < s->linesize; x += step)
                        imin = FFMIN(imin, src[x + offset]);
                    srcrow += in->linesize[0];
                }
            }
            if (imax < 0) {
                srcrow = in->data[0];
                imax = 0;
                for (y = 0; y < inlink->h; y++) {
                    const uint16_t *src = (const uint16_t *)srcrow;
                    for (x = 0; x < s->linesize; x += step)
                        imax = FFMAX(imax, src[x + offset]);
                    srcrow += in->linesize[0];
                }
            }

            srcrow = in->data[0];
            coeff = (omax - omin) / (double)(imax - imin);
            for (y = 0; y < inlink->h; y++) {
                const uint16_t *src = (const uint16_t*)srcrow;
                uint16_t *dst = (uint16_t *)dstrow;
                for (x = 0; x < s->linesize; x += step)
                    dst[x + offset] = av_clip_uint16((src[x + offset] - imin) * coeff + omin);
                dstrow += out->linesize[0];
                srcrow += in->linesize[0];
            }
        }
    }

    if (in != out)
        av_frame_free(&in);
    return ff_filter_frame(outlink, out);
}