static void my_draw_bitmap(struct vf_instance* vf, unsigned char* bitmap, int bitmap_w, int bitmap_h, int stride, int dst_x, int dst_y, unsigned color)
{
    const unsigned char y = rgba2y(color);
    const unsigned char u = rgba2u(color);
    const unsigned char v = rgba2v(color);
    const unsigned k_scale = (255 - _a(color)) * 32897; // 32897 = (1<<16)/255.0
    
    unsigned char *src, *dsty, *dstu, *dstv;
    int i, j;
    mp_image_t* dmpi = vf->dmpi;

    src = bitmap;
    dsty = dmpi->planes[0] + dst_x + dst_y * dmpi->stride[0];
    dstu = vf->priv->planes[1] + dst_x + dst_y * vf->priv->outw;
    dstv = vf->priv->planes[2] + dst_x + dst_y * vf->priv->outw;
    
    for (i = 0; i < bitmap_h; ++i) {
        unsigned char *s = src;
        unsigned char *dy = dsty;
        unsigned char *du = dstu;
        unsigned char *dv = dstv;
        
        // Process 4 pixels at a time
        for (j = 0; j < bitmap_w - 3; j += 4) {
            // Pixel 0
            unsigned k = (s[0] * k_scale) >> 16;
            dy[0] = (k*y + (255-k)*dy[0]) >> 8;
            du[0] = (k*u + (255-k)*du[0]) >> 8;
            dv[0] = (k*v + (255-k)*dv[0]) >> 8;
            
            // Pixel 1
            k = (s[1] * k_scale) >> 16;
            dy[1] = (k*y + (255-k)*dy[1]) >> 8;
            du[1] = (k*u + (255-k)*du[1]) >> 8;
            dv[1] = (k*v + (255-k)*dv[1]) >> 8;
            
            // Pixel 2
            k = (s[2] * k_scale) >> 16;
            dy[2] = (k*y + (255-k)*dy[2]) >> 8;
            du[2] = (k*u + (255-k)*du[2]) >> 8;
            dv[2] = (k*v + (255-k)*dv[2]) >> 8;
            
            // Pixel 3
            k = (s[3] * k_scale) >> 16;
            dy[3] = (k*y + (255-k)*dy[3]) >> 8;
            du[3] = (k*u + (255-k)*du[3]) >> 8;
            dv[3] = (k*v + (255-k)*dv[3]) >> 8;
            
            s += 4;
            dy += 4;
            du += 4;
            dv += 4;
        }
        
        // Process remaining pixels
        for (; j < bitmap_w; j++) {
            unsigned k = (s[0] * k_scale) >> 16;
            dy[0] = (k*y + (255-k)*dy[0]) >> 8;
            du[0] = (k*u + (255-k)*du[0]) >> 8;
            dv[0] = (k*v + (255-k)*dv[0]) >> 8;
            s++;
            dy++;
            du++;
            dv++;
        }
        
        src += stride;
        dsty += dmpi->stride[0];
        dstu += vf->priv->outw;
        dstv += vf->priv->outw;
    }
}