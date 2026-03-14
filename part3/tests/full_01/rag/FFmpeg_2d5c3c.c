static int gdv_decode_frame(AVCodecContext *avctx, void *data,
                            int *got_frame, AVPacket *avpkt)
{
    GDVContext *gdv = avctx->priv_data;
    GetByteContext *gb = &gdv->gb;
    PutByteContext *pb = &gdv->pb;
    AVFrame *frame = data;
    int ret, i, pal_size;
    const uint8_t *pal = av_packet_get_side_data(avpkt, AV_PKT_DATA_PALETTE, &pal_size);
    int compression;
    unsigned flags;
    uint8_t *dst;

    bytestream2_init(gb, avpkt->data, avpkt->size);
    bytestream2_init_writer(pb, gdv->frame, gdv->frame_size);

    flags = bytestream2_get_le32(gb);
    compression = flags & 0xF;

    if (compression == 4 || compression == 7 || compression > 8)
        return AVERROR_INVALIDDATA;

    if ((ret = ff_get_buffer(avctx, frame, 0)) < 0)
        return ret;
    if (pal && pal_size == AVPALETTE_SIZE)
        memcpy(gdv->pal, pal, AVPALETTE_SIZE);

    rescale(gdv, gdv->frame, avctx->width, avctx->height,
            !!(flags & 0x10), !!(flags & 0x20));

    switch (compression) {
    case 1:
        memset(gdv->frame + PREAMBLE_SIZE, 0, gdv->frame_size - PREAMBLE_SIZE);
    case 0:
        if (bytestream2_get_bytes_left(gb) < 256*3)
            return AVERROR_INVALIDDATA;
        for (i = 0; i < 256; i++) {
            unsigned r = bytestream2_get_byte(gb);
            unsigned g = bytestream2_get_byte(gb);
            unsigned b = bytestream2_get_byte(gb);
            gdv->pal[i] = 0xFFU << 24 | r << 18 | g << 10 | b << 2;
        }
        break;
    case 2:
        ret = decompress_2(avctx);
        break;
    case 3:
        break;
    case 5:
        ret = decompress_5(avctx, flags >> 8);
        break;
    case 6:
        ret = decompress_68(avctx, flags >> 8, 0);
        break;
    case 8:
        ret = decompress_68(avctx, flags >> 8, 1);
        break;
    default:
        av_assert0(0);
    }
    if (ret < 0)
        return ret;

    memcpy(frame->data[1], gdv->pal, AVPALETTE_SIZE);
    dst = frame->data[0];
    {
        const uint8_t *src = gdv->frame + PREAMBLE_SIZE;
        const int width = avctx->width;
        const int height = avctx->height;
        const int src_stride = gdv->scale_v ? width/2 : width;
        const int dst_stride = frame->linesize[0];
        int y, x;

        for (y = 0; y < height; y++) {
            if (!gdv->scale_v && !gdv->scale_h) {
                memcpy(dst, src, width);
                src += width;
            } else if (!gdv->scale_v) {
                if (gdv->scale_h && (y & 1)) {
                    memcpy(dst, src, width);
                    src += width;
                }
            } else {
                const uint8_t *src_row = src;
                if (!gdv->scale_h || (y & 1)) {
                    for (x = 0; x < width - 1; x += 2) {
                        dst[x] = dst[x+1] = src_row[x>>1];
                    }
                    if (width & 1) {
                        dst[width-1] = src_row[(width-1)>>1];
                    }
                    src += src_stride;
                }
            }
            dst += dst_stride;
        }
    }

    *got_frame = 1;
    return ret < 0 ? ret : avpkt->size;
}