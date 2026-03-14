static int filter_frame(AVFilterLink *inlink, AVFrame *frame)
{
    AVFilterContext *ctx = inlink->dst;
    CropDetectContext *s = ctx->priv;
    int bpp = s->max_pixsteps[0];
    int w, h, x, y, shrink_by;
    AVDictionary **metadata;
    int limit = round(s->limit);

    // ignore first 2 frames - they may be empty
    if (++s->frame_nb > 0) {
        metadata = avpriv_frame_get_metadatap(frame);

        // Reset the crop area every reset_count frames, if reset_count is > 0
        if (s->reset_count > 0 && s->frame_nb > s->reset_count) {
            s->x1 = frame->width  - 1;
            s->y1 = frame->height - 1;
            s->x2 = 0;
            s->y2 = 0;
            s->frame_nb = 1;
        }

        // Parallel boundary detection
        #pragma omp parallel sections
        {
            #pragma omp section
            {
                int outliers = 0, last_y, y;
                for (last_y = y = 0; y < s->y1; y++) {
                    if (checkline(ctx, frame->data[0] + frame->linesize[0] * y, bpp, frame->width, bpp) > limit) {
                        if (++outliers > s->max_outliers) {
                            s->y1 = last_y;
                            break;
                        }
                    } else {
                        last_y = y + 1;
                    }
                }
            }
            
            #pragma omp section
            {
                int outliers = 0, last_y, y;
                for (last_y = y = frame->height - 1; y > FFMAX(s->y2, s->y1); y--) {
                    if (checkline(ctx, frame->data[0] + frame->linesize[0] * y, bpp, frame->width, bpp) > limit) {
                        if (++outliers > s->max_outliers) {
                            s->y2 = last_y;
                            break;
                        }
                    } else {
                        last_y = y - 1;
                    }
                }
            }
            
            #pragma omp section
            {
                int outliers = 0, last_y, y;
                for (last_y = y = 0; y < s->x1; y++) {
                    if (checkline(ctx, frame->data[0] + bpp * y, frame->linesize[0], frame->height, bpp) > limit) {
                        if (++outliers > s->max_outliers) {
                            s->x1 = last_y;
                            break;
                        }
                    } else {
                        last_y = y + 1;
                    }
                }
            }
            
            #pragma omp section
            {
                int outliers = 0, last_y, y;
                for (last_y = y = frame->width - 1; y > FFMAX(s->x2, s->x1); y--) {
                    if (checkline(ctx, frame->data[0] + bpp * y, frame->linesize[0], frame->height, bpp) > limit) {
                        if (++outliers > s->max_outliers) {
                            s->x2 = last_y;
                            break;
                        }
                    } else {
                        last_y = y - 1;
                    }
                }
            }
        }

        // round x and y (up), important for yuv colorspaces
        x = (s->x1+1) & ~1;
        y = (s->y1+1) & ~1;

        w = s->x2 - x + 1;
        h = s->y2 - y + 1;

        if (s->round <= 1)
            s->round = 16;
        if (s->round % 2)
            s->round *= 2;

        shrink_by = w % s->round;
        w -= shrink_by;
        x += (shrink_by/2 + 1) & ~1;

        shrink_by = h % s->round;
        h -= shrink_by;
        y += (shrink_by/2 + 1) & ~1;

        SET_META("lavfi.cropdetect.x1", s->x1);
        SET_META("lavfi.cropdetect.x2", s->x2);
        SET_META("lavfi.cropdetect.y1", s->y1);
        SET_META("lavfi.cropdetect.y2", s->y2);
        SET_META("lavfi.cropdetect.w",  w);
        SET_META("lavfi.cropdetect.h",  h);
        SET_META("lavfi.cropdetect.x",  x);
        SET_META("lavfi.cropdetect.y",  y);

        av_log(ctx, AV_LOG_INFO,
               "x1:%d x2:%d y1:%d y2:%d w:%d h:%d x:%d y:%d pts:%"PRId64" t:%f crop=%d:%d:%d:%d\n",
               s->x1, s->x2, s->y1, s->y2, w, h, x, y, frame->pts,
               frame->pts == AV_NOPTS_VALUE ? -1 : frame->pts * av_q2d(inlink->time_base),
               w, h, x, y);
    }

    return ff_filter_frame(inlink->dst->outputs[0], frame);
}