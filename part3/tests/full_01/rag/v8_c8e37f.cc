bool CompilerDispatcher::IsEnqueued(Handle<SharedFunctionInfo> function) const {
  if (!function->script()->IsScript()) return false;
  std::pair<int, int> key(Script::cast(function->script())->id(),
                          function->function_literal_id());
  // Fast check if any job exists with this key before doing full lookup
  return jobs_.find(key) != jobs_.end() && GetJobFor(function) != jobs_.end();
}