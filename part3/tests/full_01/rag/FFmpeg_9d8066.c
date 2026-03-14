/**
 * Calculate a 512-point MDCT
 * @param out 256 output frequency coefficients
 * @param in  512 windowed input audio samples
 */
static void mdct512(int32_t *out, int16_t *in)
{
    IComplex x[MDCT_SAMPLES/4];
    const int16_t *in1, *in2;
    int16_t *rot = (int16_t *)x; // reuse x as temporary buffer
    int i;

    /* shift to simplify computations using pointer arithmetic */
    in1 = in + 3*MDCT_SAMPLES/4;
    in2 = in + MDCT_SAMPLES/4;
    for (i = 0; i < MDCT_SAMPLES/4; i++)
        rot[i] = -*in1++;
    for (; i < MDCT_SAMPLES; i++)
        rot[i] = *in2++;

    /* pre rotation with loop unrolling */
    for (i = 0; i < MDCT_SAMPLES/4; i++) {
        const int idx1 = 2*i;
        const int idx2 = MDCT_SAMPLES - 1 - 2*i;
        const int idx3 = MDCT_SAMPLES/2 + 2*i;
        const int idx4 = MDCT_SAMPLES/2 - 1 - 2*i;
        int re =  ((int)rot[idx1] - (int)rot[idx2]) >> 1;
        int im = -((int)rot[idx3] - (int)rot[idx4]) >> 1;
        CMUL(x[i].re, x[i].im, re, im, -xcos1[i], xsin1[i]);
    }

    fft(x, MDCT_NBITS - 2);

    /* post rotation with loop unrolling */
    for (i = 0; i < MDCT_SAMPLES/4; i++) {
        int re = x[i].re;
        int im = x[i].im;
        int re1, im1;
        CMUL(re1, im1, re, im, xsin1[i], xcos1[i]);
        out[2*i] = im1;
        out[MDCT_SAMPLES/2-1-2*i] = re1;
    }
}