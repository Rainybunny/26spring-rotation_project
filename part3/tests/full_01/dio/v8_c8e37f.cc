bool CompilerDispatcher::IsEnqueued(Handle<SharedFunctionInfo> function) const {
  if (!function->script()->IsScript()) return false;
  std::pair<int, int> key(Script::cast(function->script())->id(),
                          function->function_literal_id());
  auto range = jobs_.equal_range(key);
  for (auto job = range.first; job != range.second; ++job) {
    if (job->second->IsAssociatedWith(function)) return true;
  }
  return false;
}