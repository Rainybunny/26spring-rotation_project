void AddInternalKey(TableConstructor* c, const std::string& prefix,
                    int suffix_len = 800) {
  thread_local Random rnd(1023);
  
  // Pre-allocate space for the full key to avoid reallocations
  std::string key;
  key.reserve(prefix.size() + suffix_len + 8); // +8 for InternalKey encoding
  
  // Build the key directly
  key.append(prefix);
  test::RandomString(&rnd, suffix_len, &key); // Append random suffix directly
  
  // Create and encode InternalKey more efficiently
  InternalKey k(Slice(key), 0, kTypeValue);
  std::string encoded_key;
  k.EncodeTo(&encoded_key);
  
  c->Add(encoded_key, "v");
}