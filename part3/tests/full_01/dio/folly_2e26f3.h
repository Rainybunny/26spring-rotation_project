void wake() {
    if (!signaled_.load(std::memory_order_relaxed)) {
      std::unique_lock<std::mutex> nodeLock(mutex_);
      // Check again in case another thread signaled while we were waiting for lock
      if (!signaled_.load(std::memory_order_relaxed)) {
        signaled_.store(true, std::memory_order_relaxed);
        cond_.notify_one();
      }
    }
  }