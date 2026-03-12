void wake() {
    if (!signaled_.exchange(true, std::memory_order_relaxed)) {
      std::unique_lock<std::mutex> nodeLock(mutex_);
      cond_.notify_one();
    }
  }