// Gets the resulting type from a broadcast between two types.
static Type GetBroadcastType(Type x, Type y, Type element_type,
                             DenseIntElementsAttr broadcast_dimensions_attr) {
  auto x_ranked = x.dyn_cast<RankedTensorType>();
  auto y_ranked = y.dyn_cast<RankedTensorType>();
  if (!x_ranked || !y_ranked) {
    return UnrankedTensorType::get(element_type);
  }

  auto shape_x = x_ranked.getShape();
  auto shape_y = y_ranked.getShape();
  const auto x_size = shape_x.size();
  const auto y_size = shape_y.size();

  // Handle equal rank case first
  if (x_size == y_size) {
    llvm::SmallVector<int64_t, 4> out_shape(x_size);
    for (int i = 0; i < x_size; ++i) {
      const auto x_val = shape_x[i];
      const auto y_val = shape_y[i];
      out_shape[i] = (x_val == -1 || y_val == -1) ? -1 : std::max(x_val, y_val);
    }
    return RankedTensorType::get(out_shape, element_type);
  }

  // Handle different rank cases
  const bool x_larger = x_size > y_size;
  const auto& shape_large = x_larger ? shape_x : shape_y;
  const auto& shape_small = x_larger ? shape_y : shape_x;
  const auto small_size = shape_small.size();

  llvm::SmallVector<int64_t, 4> broadcast_dimensions;
  if (broadcast_dimensions_attr) {
    // Explicit broadcast dimensions
    broadcast_dimensions.reserve(small_size);
    for (const APInt& int_value : broadcast_dimensions_attr.getValues<APInt>()) {
      broadcast_dimensions.push_back(int_value.getSExtValue());
    }
    if (broadcast_dimensions.size() != small_size) {
      return UnrankedTensorType::get(element_type);
    }
  } else {
    // Numpy-style broadcasting - most common case
    const auto offset = shape_large.size() - small_size;
    broadcast_dimensions.reserve(small_size);
    for (int64_t i = 0; i < small_size; ++i) {
      broadcast_dimensions.push_back(offset + i);
    }
  }

  // Initialize output with large shape
  llvm::SmallVector<int64_t, 4> out_shape(shape_large.begin(), shape_large.end());

  // Update dimensions
  for (int64_t i = 0; i < small_size; ++i) {
    const auto dim = broadcast_dimensions[i];
    const auto old_val = out_shape[dim];
    const auto new_val = shape_small[i];
    if (old_val != -1 && (new_val == -1 || new_val > old_val)) {
      out_shape[dim] = new_val;
    }
  }

  return RankedTensorType::get(out_shape, element_type);
}