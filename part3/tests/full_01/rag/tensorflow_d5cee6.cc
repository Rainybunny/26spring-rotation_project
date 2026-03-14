template <typename T>
void Col2im(const T* col_data, const int depth, const int height,
            const int width, const int filter_h, const int filter_w,
            const int pad_t, const int pad_l, const int pad_b, const int pad_r,
            const int stride_h, const int stride_w, T* im_data) {
  int height_col = (height + pad_t + pad_b - filter_h) / stride_h + 1;
  int width_col = (width + pad_l + pad_r - filter_w) / stride_w + 1;
  int h_pad = -pad_t;
  
  // Determine vectorization width based on type and architecture
  constexpr int kVecWidth = sizeof(T) == 4 ? 8 : (sizeof(T) == 2 ? 16 : 4);
  
  for (int h = 0; h < height_col; ++h) {
    int w_pad = -pad_l;
    for (int w = 0; w < width_col; ++w) {
      T* im_patch_data = im_data + (h_pad * width + w_pad) * depth;
      for (int ih = h_pad; ih < h_pad + filter_h; ++ih) {
        for (int iw = w_pad; iw < w_pad + filter_w; ++iw) {
          if (ih >= 0 && ih < height && iw >= 0 && iw < width) {
            // Vectorized part
            int i = 0;
            for (; i + kVecWidth <= depth; i += kVecWidth) {
              auto im_vec = Eigen::internal::ploadu<T>(im_patch_data + i);
              auto col_vec = Eigen::internal::ploadu<T>(col_data + i);
              auto res_vec = Eigen::internal::padd(im_vec, col_vec);
              Eigen::internal::pstoreu<T>(im_patch_data + i, res_vec);
            }
            // Remainder
            for (; i < depth; ++i) {
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