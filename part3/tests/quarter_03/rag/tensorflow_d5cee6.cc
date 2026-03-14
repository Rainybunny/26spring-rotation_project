template <typename T>
void Col2im(const T* col_data, const int depth, const int height,
            const int width, const int filter_h, const int filter_w,
            const int pad_t, const int pad_l, const int pad_b, const int pad_r,
            const int stride_h, const int stride_w, T* im_data) {
  int height_col = (height + pad_t + pad_b - filter_h) / stride_h + 1;
  int width_col = (width + pad_l + pad_r - filter_w) / stride_w + 1;
  int h_pad = -pad_t;
  
  using Vec = Eigen::Array<T, Eigen::Dynamic, 1>;
  const int vectorized_size = depth - (depth % Vec::SizeAtCompileTime);
  
  for (int h = 0; h < height_col; ++h) {
    int w_pad = -pad_l;
    for (int w = 0; w < width_col; ++w) {
      T* im_patch_data = im_data + (h_pad * width + w_pad) * depth;
      for (int ih = h_pad; ih < h_pad + filter_h; ++ih) {
        for (int iw = w_pad; iw < w_pad + filter_w; ++iw) {
          if (ih >= 0 && ih < height && iw >= 0 && iw < width) {
            // Vectorized part
            int i = 0;
            for (; i < vectorized_size; i += Vec::SizeAtCompileTime) {
              Vec im_vec = Eigen::Map<const Vec>(im_patch_data + i);
              Vec col_vec = Eigen::Map<const Vec>(col_data + i);
              Eigen::Map<Vec>(im_patch_data + i) = im_vec + col_vec;
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