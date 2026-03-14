static double cT[64]; // Transposed cosine matrix

static void init_idct(void)
{
    int i, j;

    for (i = 0; i < 8; i++) {
        double s = i == 0 ? sqrt(0.125) : 0.5;

        for (j = 0; j < 8; j++) {
            c[i*8+j] = s*cos((M_PI/8.0)*i*(j+0.5));
            cT[j*8+i] = c[i*8+j]; // Store transposed version
        }
    }
}

static void idct(uint8_t *dst, int dst_linesize, int src[64])
{
    int i, j, k;
    double tmp[64];

    // First pass (matrix multiplication with c)
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            double sum = 0.0;
            const double *c_ptr = &c[j]; // Access column-wise

            for (k = 0; k < 8; k++) {
                sum += c_ptr[k*8] * src[8*i+k];
            }
            tmp[8*i+j] = sum;
        }
    }

    // Second pass (matrix multiplication with cT)
    for (j = 0; j < 8; j++) {
        for (i = 0; i < 8; i++) {
            double sum = 0.0;
            const double *cT_ptr = &cT[i]; // Access column-wise

            for (k = 0; k < 8; k++) {
                sum += cT_ptr[k*8] * tmp[8*k+j];
            }
            dst[dst_linesize*i + j] = av_clip_uint8((int)floor(sum+0.5));
        }
    }
}