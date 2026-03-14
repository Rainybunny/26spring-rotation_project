void AddInternalKey(TableConstructor* c, const std::string prefix,
                    int suffix_len = 800) {
  static Random rnd(1023);
  // Pre-allocate buffer large enough for prefix + suffix + internal key overhead
  std::string key;
  key.reserve(prefix.size() + suffix_len + 8);
  key.append(prefix);
  test::RandomString(&rnd, suffix_len, &key);  // Append directly to key buffer
  
  InternalKey k(key, 0, kTypeValue);
  c->Add(k.Encode().ToString(), "v");
}