static int plot_spectrum_column(AVFilterLink *inlink, AVFrame *insamples)
{
    int ret;
    AVFilterContext *ctx = inlink->dst;
    AVFilterLink *outlink = ctx->outputs[0];
    ShowSpectrumContext *s = ctx->priv;
    AVFrame *outpicref = s->outpicref;
    const double w = s->win_scale;
    const float g = s->gain;
    int h = s->orientation == VERTICAL ? s->channel_height : s->channel_width;
    int ch, plane, x, y;

    /* fill a new spectrum column */
    clear_combine_buffer(s, s->orientation == VERTICAL ? outlink->h : outlink->w);

    for (ch = 0; ch < s->nb_display_channels; ch++) {
        float *magnitudes = s->magnitudes[ch];
        float yf, uf, vf;

        /* decide color range */
        if (s->mode == COMBINED) {
            yf = 256.0f / s->nb_display_channels;
            if (s->color_mode == CHANNEL) {
                uf = yf * M_PI;
                vf = yf * M_PI;
                if (s->nb_display_channels > 1) {
                    float angle = (2 * M_PI * ch) / s->nb_display_channels;
                    uf *= 0.5 * sinf(angle);
                    vf *= 0.5 * cosf(angle);
                } else {
                    uf = vf = 0.0f;
                }
            } else {
                uf = vf = yf;
            }
        } else { /* SEPARATE */
            yf = uf = vf = 256.0f;
        }
        uf *= s->saturation;
        vf *= s->saturation;

        /* draw the channel */
        for (y = 0; y < h; y++) {
            int row = (s->mode == COMBINED) ? y : ch * h + y;
            float *out = &s->combine_buffer[3 * row];
            float a = g * w * magnitudes[y];

            /* apply scale */
            switch (s->scale) {
            case SQRT:    a = sqrtf(a); break;
            case CBRT:    a = cbrtf(a); break;
            case FOURTHRT:a = powf(a, 0.25f); break;
            case FIFTHRT: a = powf(a, 0.20f); break;
            case LOG:     a = 1 + log10f(av_clipf(a * w, 1e-6f, 1)) / 6; break;
            case LINEAR:  break;
            default:      av_assert0(0);
            }

            pick_color(s, yf, uf, vf, a, out);
        }
    }

    av_frame_make_writable(s->outpicref);
    if (s->orientation == VERTICAL) {
        if (s->sliding == SCROLL || s->sliding == RSCROLL) {
            int scroll_dir = s->sliding == SCROLL ? -1 : 1;
            for (plane = 0; plane < 3; plane++) {
                uint8_t *data = outpicref->data[plane];
                int linesize = outpicref->linesize[plane];
                for (y = 0; y < outlink->h; y++) {
                    uint8_t *p = data + y * linesize;
                    if (scroll_dir < 0) {
                        memmove(p, p + 1, outlink->w - 1);
                    } else {
                        memmove(p + 1, p, outlink->w - 1);
                    }
                }
            }
            s->xpos = scroll_dir < 0 ? outlink->w - 1 : 0;
        }
        
        for (plane = 0; plane < 3; plane++) {
            uint8_t *p = outpicref->data[plane] + (outlink->h - 1) * outpicref->linesize[plane] + s->xpos;
            for (y = 0; y < outlink->h; y++) {
                *p = lrintf(av_clipf(s->combine_buffer[3 * y + plane], 0, 255));
                p -= outpicref->linesize[plane];
            }
        }
    } else {
        if (s->sliding == SCROLL || s->sliding == RSCROLL) {
            int scroll_dir = s->sliding == SCROLL ? -1 : 1;
            for (plane = 0; plane < 3; plane++) {
                uint8_t *data = outpicref->data[plane];
                int linesize = outpicref->linesize[plane];
                if (scroll_dir < 0) {
                    for (y = 1; y < outlink->h; y++) {
                        memcpy(data + (y-1)*linesize, data + y*linesize, outlink->w);
                    }
                } else {
                    for (y = outlink->h - 1; y >= 1; y--) {
                        memcpy(data + y*linesize, data + (y-1)*linesize, outlink->w);
                    }
                }
            }
            s->xpos = scroll_dir < 0 ? outlink->h - 1 : 0;
        }

        for (plane = 0; plane < 3; plane++) {
            uint8_t *p = outpicref->data[plane] + s->xpos * outpicref->linesize[plane];
            for (x = 0; x < outlink->w; x++) {
                *p++ = lrintf(av_clipf(s->combine_buffer[3 * x + plane], 0, 255));
            }
        }
    }

    if (s->sliding != FULLFRAME || s->xpos == 0)
        outpicref->pts = insamples->pts;

    s->xpos++;
    if ((s->orientation == VERTICAL && s->xpos >= outlink->w) ||
        (s->orientation == HORIZONTAL && s->xpos >= outlink->h))
        s->xpos = 0;
        
    if (!s->single_pic && (s->sliding != FULLFRAME || s->xpos == 0)) {
        ret = ff_filter_frame(outlink, av_frame_clone(s->outpicref));
        if (ret < 0)
            return ret;
    }

    return s->win_size;
}