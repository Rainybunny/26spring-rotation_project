void AddInternalKey(TableConstructor* c, const std::string prefix,
                    int suffix_len = 800) {
  static Random rnd(1023);
  // Cache for random suffixes to avoid regeneration
  static std::vector<std::string> suffix_cache;
  static const size_t cache_size = 100;
  static std::mutex cache_mutex;
  
  // Initialize cache if empty
  if (suffix_cache.empty()) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    if (suffix_cache.empty()) {
      suffix_cache.reserve(cache_size);
      for (size_t i = 0; i < cache_size; ++i) {
        suffix_cache.emplace_back(RandomString(&rnd, suffix_len));
      }
    }
  }
  
  // Get random suffix from cache
  size_t index = rnd.Uniform(cache_size);
  InternalKey k(prefix + suffix_cache[index], 0, kTypeValue);
  c->Add(k.Encode().ToString(), "v");
}