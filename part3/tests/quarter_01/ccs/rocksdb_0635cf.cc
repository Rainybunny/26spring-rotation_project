void AddInternalKey(TableConstructor* c, const std::string& prefix,
                    int suffix_len = 800) {
  static Random rnd(1023);
  static std::string random_suffix;
  static std::mutex mutex;
  
  // Initialize random suffix once
  {
    std::lock_guard<std::mutex> lock(mutex);
    if (random_suffix.empty()) {
      random_suffix = RandomString(&rnd, suffix_len);
    }
  }

  // Reuse buffers to avoid allocations
  thread_local std::string key_buffer;
  key_buffer = prefix;
  key_buffer += random_suffix;
  
  InternalKey k(key_buffer, 0, kTypeValue);
  c->Add(k.Encode().ToString(), "v");
}