void wake() {
    // Fast path check if already signaled
    if (!signaled_.load(std::memory_order_relaxed)) {
      std::unique_lock<std::mutex> nodeLock(mutex_);
      // Check again under lock to avoid race
      if (!signaled_.exchange(true, std::memory_order_relaxed)) {
        cond_.notify_one();
      }
    }
  }