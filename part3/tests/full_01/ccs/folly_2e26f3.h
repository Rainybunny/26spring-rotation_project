void wake() {
    if (!signaled_.load(std::memory_order_relaxed)) {
      std::unique_lock<std::mutex> nodeLock(mutex_);
      if (!signaled_.load(std::memory_order_relaxed)) {
        signaled_.store(true, std::memory_order_relaxed);
        cond_.notify_one();
      }
    }
  }