void wake() {
    if (!signaled_.load(std::memory_order_relaxed)) {
      std::unique_lock<std::mutex> nodeLock(mutex_);
      signaled_ = true;
      cond_.notify_one();
    }
  }