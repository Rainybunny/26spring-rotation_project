v8::AllocationProfile::Node* SamplingHeapProfiler::TranslateAllocationNode(
    AllocationProfile* profile, SamplingHeapProfiler::AllocationNode* node,
    const std::map<int, Handle<Script>>& scripts) {
  // Cache the empty script name string
  static Local<v8::String> empty_script_name =
      ToApiHandle<v8::String>(isolate_->factory()->InternalizeUtf8String(""));

  node->pinned_ = true;
  Local<v8::String> script_name = empty_script_name;
  int line = v8::AllocationProfile::kNoLineNumberInfo;
  int column = v8::AllocationProfile::kNoColumnNumberInfo;
  std::vector<v8::AllocationProfile::Allocation> allocations;
  allocations.reserve(node->allocations_.size());
  if (node->script_id_ != v8::UnboundScript::kNoScriptId) {
    auto script_it = scripts.find(node->script_id_);
    if (script_it != scripts.end()) {
      Handle<Script> script = script_it->second;
      if (!script.is_null()) {
        if (script->name().IsName()) {
          Name name = Name::cast(script->name());
          script_name = ToApiHandle<v8::String>(
              isolate_->factory()->InternalizeUtf8String(names_->GetName(name)));
        }
        line = 1 + Script::GetLineNumber(script, node->script_position_);
        column = 1 + Script::GetColumnNumber(script, node->script_position_);
      }
    }
  }
  for (auto alloc : node->allocations_) {
    allocations.push_back(ScaleSample(alloc.first, alloc.second));
  }

  profile->nodes_.push_back(v8::AllocationProfile::Node{
      ToApiHandle<v8::String>(
          isolate_->factory()->InternalizeUtf8String(node->name_)),
      script_name, node->script_id_, node->script_position_, line, column,
      node->id_, std::vector<v8::AllocationProfile::Node*>(), allocations});
  v8::AllocationProfile::Node* current = &profile->nodes_.back();
  for (const auto& it : node->children_) {
    current->children.push_back(
        TranslateAllocationNode(profile, it.second.get(), scripts));
  }
  node->pinned_ = false;
  return current;
}