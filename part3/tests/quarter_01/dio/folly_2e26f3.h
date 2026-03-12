void wake() {
    signaled_.store(true, std::memory_order_release);
    cond_.notify_one();
  }