void AddInternalKey(TableConstructor* c, const std::string prefix,
                    int suffix_len = 800) {
  static Random rnd(1023);
  static std::string random_buf;
  random_buf.resize(800);
  test::RandomString(&rnd, 800, &random_buf);
  InternalKey k(prefix + random_buf, 0, kTypeValue);
  c->Add(k.Encode().ToString(), "v");
}