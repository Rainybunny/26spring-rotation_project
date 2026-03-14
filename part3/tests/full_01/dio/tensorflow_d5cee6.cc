template <typename T>
void Col2im(const T* col_data, const int depth, const int height,
            const int width, const int filter_h, const int filter_w,
            const int pad_t, const int pad_l, const int pad_b, const int pad_r,
            const int stride_h, const int stride_w, T* im_data) {
  int height_col = (height + pad_t + pad_b - filter_h) / stride_h + 1;
  int width_col = (width + pad_l + pad_r - filter_w) / stride_w + 1;
  int h_pad = -pad_t;
  
  // Vectorized version for float types
  if (std::is_same<T, float>::value) {
    constexpr int kVecSize = 8; // AVX width for floats
    const int depth_vec = depth / kVecSize;
    const int depth_remain = depth % kVecSize;
    
    for (int h = 0; h < height_col; ++h) {
      int w_pad = -pad_l;
      for (int w = 0; w < width_col; ++w) {
        T* im_patch_data = im_data + (h_pad * width + w_pad) * depth;
        for (int ih = h_pad; ih < h_pad + filter_h; ++ih) {
          for (int iw = w_pad; iw < w_pad + filter_w; ++iw) {
            if (ih >= 0 && ih < height && iw >= 0 && iw < width) {
              // Vectorized part
              for (int i = 0; i < depth_vec; ++i) {
                auto* src = col_data + i * kVecSize;
                auto* dst = im_patch_data + i * kVecSize;
                for (int v = 0; v < kVecSize; ++v) {
                  dst[v] += src[v];
                }
              }
              // Remainder
              for (int i = depth_vec * kVecSize; i < depth; ++i) {
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
  } else {
    // Original scalar version for non-float types
    for (int h = 0; h < height_col; ++h) {
      int w_pad = -pad_l;
      for (int w = 0; w < width_col; ++w) {
        T* im_patch_data = im_data + (h_pad * width + w_pad) * depth;
        for (int ih = h_pad; ih < h_pad + filter_h; ++ih) {
          for (int iw = w_pad; iw < w_pad + filter_w; ++iw) {
            if (ih >= 0 && ih < height && iw >= 0 && iw < width) {
              for (int i = 0; i < depth; ++i) {
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
}