void AddInternalKey(TableConstructor* c, const std::string prefix,
                    int suffix_len = 800) {
  static Random rnd(1023);
  // Cache the random suffixes to avoid regeneration
  static std::unordered_map<std::string, std::string> suffix_cache;
  auto it = suffix_cache.find(prefix);
  if (it == suffix_cache.end()) {
    it = suffix_cache.emplace(prefix, RandomString(&rnd, 800)).first;
  }
  InternalKey k(prefix + it->second, 0, kTypeValue);
  c->Add(k.Encode().ToString(), "v");
}