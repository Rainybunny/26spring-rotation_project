void AddInternalKey(TableConstructor* c, const std::string prefix,
                    int suffix_len = 800) {
  // Static initialization happens only once
  static Random rnd(1023);
  static std::string random_buf;
  random_buf.resize(800);  // Pre-allocate buffer
  
  // Reuse the same buffer for random string generation
  test::RandomString(&rnd, 800, &random_buf);
  InternalKey k(prefix + random_buf, 0, kTypeValue);
  c->Add(k.Encode().ToString(), "v");
}