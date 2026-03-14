static FailureOr<Attribute> GetLayout(const Type &type, OpBuilder &rewriter) {
    auto i64_type = rewriter.getIntegerType(64);
    if (type.isa<TupleType>()) {
      auto tuple_type = type.dyn_cast<TupleType>();
      std::vector<mlir::Attribute> v;
      auto types = tuple_type.getTypes();
      v.reserve(types.size());
      for (const mlir::Type &t : types) {
        auto layout = GetLayout(t, rewriter);
        if (failed(layout)) return failure();
        v.push_back(layout.getValue());
      }
      ArrayRef<Attribute> shape(v);
      return rewriter.getArrayAttr(shape);
    } else if (auto t = type.dyn_cast<RankedTensorType>()) {
      if (!t.hasStaticShape()) return failure();
      auto layout = GetTPUInfeedLayoutFromAPI(t);
      std::vector<int64_t> minor_to_major;
      if (succeeded(layout)) {
        minor_to_major = layout.getValue();
      } else {
        minor_to_major.resize(t.getRank());
        std::iota(minor_to_major.begin(), minor_to_major.end(), 0);
        std::sort(minor_to_major.begin(), minor_to_major.end(),
                  [=](int64_t a, int64_t b) {
                    int64_t da = t.getDimSize(a);
                    int64_t db = t.getDimSize(b);
                    return da > db || (da == db && a > b);
                  });
      }
      std::vector<Attribute> elements;
      elements.reserve(minor_to_major.size());
      for (auto e : minor_to_major) {
        elements.push_back(rewriter.getIntegerAttr(i64_type, e));
      }
      return rewriter.getArrayAttr(elements);
    } else {
      return rewriter.getUnitAttr();  // e.g. tokens
    }
  }