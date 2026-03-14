void RuntimeScheduler_Modern::scheduleTask(std::shared_ptr<Task> task) {
  bool shouldScheduleEventLoop = false;

  {
    std::unique_lock lock(schedulingMutex_);
    shouldScheduleEventLoop = taskQueue_.empty() && !isEventLoopScheduled_;
    if (shouldScheduleEventLoop) {
      isEventLoopScheduled_ = true;
    }
    taskQueue_.push(task);
  }

  if (shouldScheduleEventLoop) {
    scheduleEventLoop();
  }
}