void wake() {
    // Fast path: check if already signaled without lock
    if (signaled_.load(std::memory_order_relaxed)) {
      return;
    }
    
    // Slow path: need to actually signal
    std::unique_lock<std::mutex> nodeLock(mutex_);
    if (!signaled_.exchange(true, std::memory_order_relaxed)) {
      cond_.notify_one();
    }
  }