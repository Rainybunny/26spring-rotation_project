template <typename T>
void Col2im(const T* col_data, const int depth, const int height,
            const int width, const int filter_h, const int filter_w,
            const int pad_t, const int pad_l, const int pad_b, const int pad_r,
            const int stride_h, const int stride_w, T* im_data) {
  int height_col = (height + pad_t + pad_b - filter_h) / stride_h + 1;
  int width_col = (width + pad_l + pad_r - filter_w) / stride_w + 1;
  int h_pad = -pad_t;
  
  // Vectorized version parameters
  constexpr int simd_width = 8; // AVX2 can process 8 floats at once
  const int depth_simd = depth / simd_width;
  const int depth_remain = depth % simd_width;

  for (int h = 0; h < height_col; ++h) {
    int w_pad = -pad_l;
    for (int w = 0; w < width_col; ++w) {
      T* im_patch_data = im_data + (h_pad * width + w_pad) * depth;
      for (int ih = h_pad; ih < h_pad + filter_h; ++ih) {
        for (int iw = w_pad; iw < w_pad + filter_w; ++iw) {
          if (ih >= 0 && ih < height && iw >= 0 && iw < width) {
            // Vectorized part
            for (int i = 0; i < depth_simd * simd_width; i += simd_width) {
              // Load 8 elements from col_data and im_patch_data
              __m256 col_vec = _mm256_loadu_ps(col_data + i);
              __m256 im_vec = _mm256_loadu_ps(im_patch_data + i);
              // Add them
              __m256 result = _mm256_add_ps(col_vec, im_vec);
              // Store back
              _mm256_storeu_ps(im_patch_data + i, result);
            }
            // Remainder
            for (int i = depth_simd * simd_width; i < depth; ++i) {
              im_patch_data[i] += col_data[i];
            }
          }
          im_patch_data += depth;
          col_data += depth;
        }
        im_patch_data += depth * (width - filter_w);
      }
      w_pad += stride_w;
    }
    h_pad += stride_h;
  }
}