void wake() {
    std::unique_lock<std::mutex> nodeLock(mutex_);
    signaled_.store(true, std::memory_order_relaxed);
    cond_.notify_one();
  }