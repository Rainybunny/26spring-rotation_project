static void idct(uint8_t *dst, int dst_linesize, int src[64])
{
    int i, j, k;
    int tmp[64];
    const int scale = 1 << 10; // Fixed-point scale factor

    // Convert cosine coefficients to fixed-point
    static const int c_fixed[64] = {
        2896, 2896, 2896, 2896, 2896, 2896, 2896, 2896,
        4017, 3406, 2276,  799, -799,-2276,-3406,-4017,
        3784, 1567,-1567,-3784,-3784,-1567, 1567, 3784,
        3406, -799,-4017,-2276, 2276, 4017,  799,-3406,
        2896,-2896,-2896, 2896, 2896,-2896,-2896, 2896,
        2276,-4017,  799, 3406,-3406, -799, 4017,-2276,
        1567,-3784, 3784,-1567,-1567, 3784,-3784, 1567,
         799,-2276, 3406,-4017, 4017,-3406, 2276, -799
    };

    // First pass: 1D IDCT on rows
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            int sum = 0;
            for (k = 0; k < 8; k++) {
                sum += (c_fixed[k*8+j] * src[8*i+k] + (1 << 9)) >> 10;
            }
            tmp[8*i+j] = sum;
        }
    }

    // Second pass: 1D IDCT on columns
    for (j = 0; j < 8; j++) {
        for (i = 0; i < 8; i++) {
            int sum = 0;
            for (k = 0; k < 8; k++) {
                sum += (c_fixed[k*8+i] * tmp[8*k+j] + (1 << 9)) >> 10;
            }
            dst[dst_linesize*i + j] = av_clip_uint8((sum + 128*8) >> 10);
        }
    }
}