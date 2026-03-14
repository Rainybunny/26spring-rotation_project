void AddInternalKey(TableConstructor* c, const std::string prefix,
                    int suffix_len = 800) {
  static Random rnd(1023);
  // Pre-allocate and cache the random suffix
  static std::string random_suffix;
  if (random_suffix.empty()) {
    random_suffix.reserve(suffix_len);
    test::RandomString(&rnd, suffix_len, &random_suffix);
  }

  // Construct the key with reserved space
  std::string key;
  key.reserve(prefix.size() + random_suffix.size() + 8); // +8 for sequence number and type
  key.append(prefix);
  key.append(random_suffix);
  
  InternalKey k(key, 0, kTypeValue);
  c->Add(k.Encode().ToString(), "v");
}