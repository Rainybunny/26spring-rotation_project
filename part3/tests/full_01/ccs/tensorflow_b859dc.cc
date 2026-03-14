// Gets the resulting type from a broadcast between two types.
static Type GetBroadcastType(Type x, Type y, Type element_type,
                             DenseIntElementsAttr broadcast_dimensions_attr) {
  auto x_ranked = x.dyn_cast<RankedTensorType>();
  auto y_ranked = y.dyn_cast<RankedTensorType>();
  if (!x_ranked || !y_ranked) {
    return UnrankedTensorType::get(element_type);
  }

  const auto& shape_x = x_ranked.getShape();
  const auto& shape_y = y_ranked.getShape();

  if (shape_x.size() == shape_y.size()) {
    llvm::SmallVector<int64_t, 4> out_shape(shape_x.size());
    for (int i = 0, e = shape_x.size(); i < e; i++) {
      auto x_val = shape_x[i];
      auto y_val = shape_y[i];
      out_shape[i] = (x_val == -1 || y_val == -1) ? -1 : std::max(x_val, y_val);
    }
    return RankedTensorType::get(out_shape, element_type);
  }

  const auto& shape_large = shape_x.size() > shape_y.size() ? shape_x : shape_y;
  const auto& shape_small = shape_x.size() <= shape_y.size() ? shape_x : shape_y;

  llvm::SmallVector<int64_t, 4> broadcast_dimensions;
  if (broadcast_dimensions_attr) {
    // Explicit broadcast dimensions.
    broadcast_dimensions.reserve(broadcast_dimensions_attr.size());
    for (const APInt& int_value : broadcast_dimensions_attr.getValues<APInt>()) {
      broadcast_dimensions.push_back(int_value.getSExtValue());
    }
    if (broadcast_dimensions.size() != shape_small.size()) {
      return UnrankedTensorType::get(element_type);
    }
  } else {
    // If no broadcast dimensions, assume "numpy" broadcasting.
    auto start = shape_large.size() - shape_small.size();
    broadcast_dimensions = llvm::to_vector<4>(llvm::seq<int64_t>(start, shape_large.size()));
  }

  llvm::SmallVector<int64_t, 4> out_shape(shape_large.begin(), shape_large.end());
  const auto broadcast_size = broadcast_dimensions.size();

  // Update according to the broadcast dimensions.
  for (size_t i = 0; i < broadcast_size; ++i) {
    auto dim = broadcast_dimensions[i];
    auto old_value = out_shape[dim];
    auto new_value = shape_small[i];
    if (old_value != -1 && (new_value == -1 || new_value > old_value)) {
      out_shape[dim] = new_value;
    }
  }

  return RankedTensorType::get(out_shape, element_type);
}