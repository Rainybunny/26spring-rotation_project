void AddInternalKey(TableConstructor* c, const std::string& prefix,
                    int suffix_len = 800) {
  static Random rnd(1023);
  static std::string random_buf;
  random_buf.resize(800);
  test::RandomString(&rnd, 800, &random_buf);
  
  std::string key_buf;
  key_buf.reserve(prefix.size() + 800 + 8); // Extra space for encoding
  key_buf.append(prefix);
  key_buf.append(random_buf);
  
  InternalKey k(key_buf, 0, kTypeValue);
  c->Add(k.Encode().ToString(), "v");
}