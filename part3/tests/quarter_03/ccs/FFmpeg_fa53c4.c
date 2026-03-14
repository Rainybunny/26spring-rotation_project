#include <immintrin.h>

static void idct(uint8_t *dst, int dst_linesize, int src[64])
{
    int i, j, k;
    double tmp[64] __attribute__((aligned(32)));
    __m256d sum_vec, c_vec, src_vec;

    // First pass (i->j)
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j += 4) {
            sum_vec = _mm256_setzero_pd();
            for (k = 0; k < 8; k++) {
                c_vec = _mm256_load_pd(&c[k*8+j]);
                src_vec = _mm256_set1_pd(src[8*i+k]);
                sum_vec = _mm256_add_pd(sum_vec, _mm256_mul_pd(c_vec, src_vec));
            }
            _mm256_store_pd(&tmp[8*i+j], sum_vec);
        }
    }

    // Second pass (j->i)
    for (j = 0; j < 8; j++) {
        for (i = 0; i < 8; i += 4) {
            sum_vec = _mm256_setzero_pd();
            for (k = 0; k < 8; k++) {
                c_vec = _mm256_load_pd(&c[k*8+i]);
                src_vec = _mm256_set1_pd(tmp[8*k+j]);
                sum_vec = _mm256_add_pd(sum_vec, _mm256_mul_pd(c_vec, src_vec));
            }
            // Store results with clipping
            double results[4];
            _mm256_store_pd(results, sum_vec);
            for (int v = 0; v < 4; v++) {
                if (i + v < 8) {
                    dst[dst_linesize*(i + v) + j] = av_clip_uint8((int)floor(results[v] + 0.5));
                }
            }
        }
    }
}