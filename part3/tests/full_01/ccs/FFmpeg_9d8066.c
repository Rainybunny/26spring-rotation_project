/**
 * Calculate a 512-point MDCT
 * @param out 256 output frequency coefficients
 * @param in  512 windowed input audio samples
 */
static void mdct512(int32_t *out, int16_t *in)
{
    int i;
    int16_t rot[MDCT_SAMPLES];
    IComplex x[MDCT_SAMPLES/4];

    /* Process first quarter and remaining 3 quarters separately for better cache locality */
    for (i = 0; i < MDCT_SAMPLES/4; i++) {
        rot[i] = -in[i + 3*MDCT_SAMPLES/4];
    }
    for (i = MDCT_SAMPLES/4; i < MDCT_SAMPLES; i++) {
        rot[i] = in[i - MDCT_SAMPLES/4];
    }

    /* Unroll pre-rotation loop by 4 */
    for (i = 0; i < MDCT_SAMPLES/4; i += 4) {
        int re0 = ((int)rot[2*i] - (int)rot[MDCT_SAMPLES-1-2*i]) >> 1;
        int im0 = -((int)rot[MDCT_SAMPLES/2+2*i] - (int)rot[MDCT_SAMPLES/2-1-2*i]) >> 1;
        CMUL(x[i].re, x[i].im, re0, im0, -xcos1[i], xsin1[i]);

        int re1 = ((int)rot[2*(i+1)] - (int)rot[MDCT_SAMPLES-1-2*(i+1)]) >> 1;
        int im1 = -((int)rot[MDCT_SAMPLES/2+2*(i+1)] - (int)rot[MDCT_SAMPLES/2-1-2*(i+1)]) >> 1;
        CMUL(x[i+1].re, x[i+1].im, re1, im1, -xcos1[i+1], xsin1[i+1]);

        int re2 = ((int)rot[2*(i+2)] - (int)rot[MDCT_SAMPLES-1-2*(i+2)]) >> 1;
        int im2 = -((int)rot[MDCT_SAMPLES/2+2*(i+2)] - (int)rot[MDCT_SAMPLES/2-1-2*(i+2)]) >> 1;
        CMUL(x[i+2].re, x[i+2].im, re2, im2, -xcos1[i+2], xsin1[i+2]);

        int re3 = ((int)rot[2*(i+3)] - (int)rot[MDCT_SAMPLES-1-2*(i+3)]) >> 1;
        int im3 = -((int)rot[MDCT_SAMPLES/2+2*(i+3)] - (int)rot[MDCT_SAMPLES/2-1-2*(i+3)]) >> 1;
        CMUL(x[i+3].re, x[i+3].im, re3, im3, -xcos1[i+3], xsin1[i+3]);
    }

    fft(x, MDCT_NBITS - 2);

    /* Unroll post-rotation loop by 4 */
    for (i = 0; i < MDCT_SAMPLES/4; i += 4) {
        int re0, im0, re1, im1;
        CMUL(re0, im0, x[i].re, x[i].im, xsin1[i], xcos1[i]);
        out[2*i] = im0;
        out[MDCT_SAMPLES/2-1-2*i] = re0;

        CMUL(re0, im0, x[i+1].re, x[i+1].im, xsin1[i+1], xcos1[i+1]);
        out[2*(i+1)] = im0;
        out[MDCT_SAMPLES/2-1-2*(i+1)] = re0;

        CMUL(re0, im0, x[i+2].re, x[i+2].im, xsin1[i+2], xcos1[i+2]);
        out[2*(i+2)] = im0;
        out[MDCT_SAMPLES/2-1-2*(i+2)] = re0;

        CMUL(re0, im0, x[i+3].re, x[i+3].im, xsin1[i+3], xcos1[i+3]);
        out[2*(i+3)] = im0;
        out[MDCT_SAMPLES/2-1-2*(i+3)] = re0;
    }
}