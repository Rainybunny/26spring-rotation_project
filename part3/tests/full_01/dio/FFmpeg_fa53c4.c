static double c[64], ct[64]; // Add transposed coefficient matrix

static void init_idct(void)
{
    int i, j;

    for (i = 0; i < 8; i++) {
        double s = i == 0 ? sqrt(0.125) : 0.5;

        for (j = 0; j < 8; j++) {
            c[i*8+j] = s*cos((M_PI/8.0)*i*(j+0.5));
            ct[j*8+i] = c[i*8+j]; // Store transposed version
        }
    }
}

static void idct(uint8_t *dst, int dst_linesize, int src[64])
{
    int i, j, k;
    double tmp[64];

    // First pass (rows) - use original coefficients
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            double sum = 0.0;
            for (k = 0; k < 8; k++)
                sum += c[k*8+j] * src[8*i+k];
            tmp[8*i+j] = sum;
        }
    }

    // Second pass (columns) - use transposed coefficients for better memory access
    for (j = 0; j < 8; j++) {
        for (i = 0; i < 8; i++) {
            double sum = 0.0;
            for (k = 0; k < 8; k++)
                sum += ct[k*8+i] * tmp[8*k+j]; // Use transposed coefficients
            dst[dst_linesize*i + j] = av_clip_uint8((int)floor(sum+0.5));
        }
    }
}