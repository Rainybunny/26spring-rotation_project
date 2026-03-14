/**
 * Calculate a 512-point MDCT
 * @param out 256 output frequency coefficients
 * @param in  512 windowed input audio samples
 */
static void mdct512(int32_t *out, int16_t *in)
{
    int i, re, im, re1, im1;
    int16_t rot[MDCT_SAMPLES];
    IComplex x[MDCT_SAMPLES/4];

    /* shift to simplify computations */
    for (i = 0; i < MDCT_SAMPLES/4; i++)
        rot[i] = -in[i + 3*MDCT_SAMPLES/4];
    for (;i < MDCT_SAMPLES; i++)
        rot[i] =  in[i -   MDCT_SAMPLES/4];

    /* pre rotation with optimized CMUL */
    for (i = 0; i < MDCT_SAMPLES/4; i++) {
        re =  ((int)rot[               2*i] - (int)rot[MDCT_SAMPLES  -1-2*i]) >> 1;
        im = -((int)rot[MDCT_SAMPLES/2+2*i] - (int)rot[MDCT_SAMPLES/2-1-2*i]) >> 1;
        /* Optimized CMUL: reduces to 2 multiplications and 2 shifts */
        x[i].re = ((re * -xcos1[i] - im * xsin1[i]) + 0x4000) >> 15;
        x[i].im = ((re * xsin1[i] + im * -xcos1[i]) + 0x4000) >> 15;
    }

    fft(x, MDCT_NBITS - 2);

    /* post rotation with optimized CMUL */
    for (i = 0; i < MDCT_SAMPLES/4; i++) {
        re = x[i].re;
        im = x[i].im;
        /* Optimized CMUL */
        re1 = ((re * xsin1[i] + im * xcos1[i]) + 0x4000) >> 15;
        im1 = ((-re * xcos1[i] + im * xsin1[i]) + 0x4000) >> 15;
        out[                 2*i] = im1;
        out[MDCT_SAMPLES/2-1-2*i] = re1;
    }
}