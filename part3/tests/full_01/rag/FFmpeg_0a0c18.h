static inline void quantize_bands(int *out, const float *in, const float *scaled,
                                  int size, float Q34, int is_signed, int maxval,
                                  const float rounding)
{
    int i;
    float qc;
    const float fmaxval = (float)maxval;
    for (i = 0; i < size; i++) {
        qc = scaled[i] * Q34;
        out[i] = (int)FFMIN(qc + rounding, fmaxval);
        if (is_signed && in[i] < 0.0f) {
            out[i] = -out[i];
        }
    }
}