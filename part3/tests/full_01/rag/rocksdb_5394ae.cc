class WrappedBloom : public FilterPolicy {
 public:
  explicit WrappedBloom(int bits_per_key) :
        filter_(NewBloomFilterPolicy(bits_per_key)),
        counter_(0) {}

  ~WrappedBloom() { delete filter_; }

  const char* Name() const override { return "WrappedRocksDbFilterPolicy"; }

  void CreateFilter(const rocksdb::Slice* keys, int n, std::string* dst)
      const override {
    std::unique_ptr<rocksdb::Slice[]> user_keys(new rocksdb::Slice[n]);
    for (int i = 0; i < n; ++i) {
      user_keys[i] = convertKey(keys[i]);
    }
    return filter_->CreateFilter(user_keys.get(), n, dst);
  }

  bool KeyMayMatch(const rocksdb::Slice& key, const rocksdb::Slice& filter)
      const override {
    counter_++;
    return filter_->KeyMayMatch(convertKey(key), filter);
  }

  uint32_t GetCounter() { return counter_; }

 private:
  inline rocksdb::Slice convertKey(const rocksdb::Slice& key) const {
    return key;
  }

  const FilterPolicy* filter_;
  mutable uint32_t counter_;
};