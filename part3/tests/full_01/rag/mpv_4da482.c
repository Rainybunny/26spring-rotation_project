static void my_draw_bitmap(struct vf_instance* vf, unsigned char* bitmap, int bitmap_w, int bitmap_h, int stride, int dst_x, int dst_y, unsigned color)
{
	unsigned char y = rgba2y(color);
	unsigned char u = rgba2u(color);
	unsigned char v = rgba2v(color);
	unsigned char opacity = 255 - _a(color);
	unsigned char *src, *dsty, *dstu, *dstv;
	int i, j;
	mp_image_t* dmpi = vf->dmpi;

	// Precompute inverse opacity (255-opacity) as it's used in all blends
	unsigned char inv_opacity = 255 - opacity;
	
	src = bitmap;
	dsty = dmpi->planes[0] + dst_x + dst_y * dmpi->stride[0];
	dstu = vf->priv->planes[1] + dst_x + dst_y * vf->priv->outw;
	dstv = vf->priv->planes[2] + dst_x + dst_y * vf->priv->outw;
	
	for (i = 0; i < bitmap_h; ++i) {
		for (j = 0; j < bitmap_w; ++j) {
			// Faster division by 255 using multiply-shift approximation
			unsigned k = ((unsigned)src[j] * opacity + 128) >> 8;
			unsigned inv_k = 255 - k;
			
			// Blend operations using multiply-shift
			dsty[j] = (k * y + inv_k * dsty[j] + 128) >> 8;
			dstu[j] = (k * u + inv_k * dstu[j] + 128) >> 8;
			dstv[j] = (k * v + inv_k * dstv[j] + 128) >> 8;
		}
		src += stride;
		dsty += dmpi->stride[0];
		dstu += vf->priv->outw;
		dstv += vf->priv->outw;
	}
}