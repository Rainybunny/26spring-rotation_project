#include <immintrin.h>

static void idct(uint8_t *dst, int dst_linesize, int src[64])
{
    double tmp[64];
    __m256d sum0, sum1, sum2, sum3;
    __m256d row0, row1, row2, row3;
    __m256d c0, c1, c2, c3;

    // First pass (row-wise)
    for (int i = 0; i < 8; i += 4) {
        for (int j = 0; j < 8; j++) {
            sum0 = _mm256_setzero_pd();
            sum1 = _mm256_setzero_pd();
            sum2 = _mm256_setzero_pd();
            sum3 = _mm256_setzero_pd();
            
            for (int k = 0; k < 8; k++) {
                __m256d coeff = _mm256_set1_pd(c[k*8+j]);
                row0 = _mm256_set1_pd(src[8*(i+0)+k]);
                row1 = _mm256_set1_pd(src[8*(i+1)+k]);
                row2 = _mm256_set1_pd(src[8*(i+2)+k]);
                row3 = _mm256_set1_pd(src[8*(i+3)+k]);
                
                sum0 = _mm256_fmadd_pd(row0, coeff, sum0);
                sum1 = _mm256_fmadd_pd(row1, coeff, sum1);
                sum2 = _mm256_fmadd_pd(row2, coeff, sum2);
                sum3 = _mm256_fmadd_pd(row3, coeff, sum3);
            }
            
            _mm256_storeu_pd(&tmp[8*(i+0)+j], sum0);
            _mm256_storeu_pd(&tmp[8*(i+1)+j], sum1);
            _mm256_storeu_pd(&tmp[8*(i+2)+j], sum2);
            _mm256_storeu_pd(&tmp[8*(i+3)+j], sum3);
        }
    }

    // Second pass (column-wise)
    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i += 4) {
            sum0 = _mm256_setzero_pd();
            sum1 = _mm256_setzero_pd();
            sum2 = _mm256_setzero_pd();
            sum3 = _mm256_setzero_pd();
            
            for (int k = 0; k < 8; k++) {
                __m256d coeff = _mm256_set1_pd(c[k*8+i]);
                row0 = _mm256_loadu_pd(&tmp[8*k+j]);
                coeff = _mm256_set1_pd(c[k*8+(i+1)]);
                row1 = _mm256_loadu_pd(&tmp[8*k+j]);
                coeff = _mm256_set1_pd(c[k*8+(i+2)]);
                row2 = _mm256_loadu_pd(&tmp[8*k+j]);
                coeff = _mm256_set1_pd(c[k*8+(i+3)]);
                row3 = _mm256_loadu_pd(&tmp[8*k+j]);
                
                sum0 = _mm256_fmadd_pd(row0, coeff, sum0);
                sum1 = _mm256_fmadd_pd(row1, coeff, sum1);
                sum2 = _mm256_fmadd_pd(row2, coeff, sum2);
                sum3 = _mm256_fmadd_pd(row3, coeff, sum3);
            }
            
            double res0 = sum0[0] + sum0[1] + sum0[2] + sum0[3];
            double res1 = sum1[0] + sum1[1] + sum1[2] + sum1[3];
            double res2 = sum2[0] + sum2[1] + sum2[2] + sum2[3];
            double res3 = sum3[0] + sum3[1] + sum3[2] + sum3[3];
            
            dst[dst_linesize*(i+0) + j] = av_clip_uint8((int)floor(res0+0.5));
            dst[dst_linesize*(i+1) + j] = av_clip_uint8((int)floor(res1+0.5));
            dst[dst_linesize*(i+2) + j] = av_clip_uint8((int)floor(res2+0.5));
            dst[dst_linesize*(i+3) + j] = av_clip_uint8((int)floor(res3+0.5));
        }
    }
}