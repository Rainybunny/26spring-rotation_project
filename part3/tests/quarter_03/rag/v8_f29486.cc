v8::AllocationProfile::Node* SamplingHeapProfiler::TranslateAllocationNode(
    AllocationProfile* profile, SamplingHeapProfiler::AllocationNode* node,
    const std::map<int, Handle<Script>>& scripts) {
  // By pinning the node we make sure its children won't get disposed if
  // a GC kicks in during the tree retrieval.
  node->pinned_ = true;
  
  // Initialize default values
  Local<v8::String> script_name =
      ToApiHandle<v8::String>(isolate_->factory()->InternalizeUtf8String(""));
  int line = v8::AllocationProfile::kNoLineNumberInfo;
  int column = v8::AllocationProfile::kNoColumnNumberInfo;
  
  // Prepare allocations vector
  std::vector<v8::AllocationProfile::Allocation> allocations;
  allocations.reserve(node->allocations_.size());
  
  // Handle script information if available
  Handle<Script> script;
  if (node->script_id_ != v8::UnboundScript::kNoScriptId) {
    auto it = scripts.find(node->script_id_);
    if (it != scripts.end()) {
      script = it->second;
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

  // Process allocations
  for (const auto& alloc : node->allocations_) {
    allocations.emplace_back(ScaleSample(alloc.first, alloc.second));
  }

  // Create new node
  profile->nodes_.emplace_back(v8::AllocationProfile::Node{
      ToApiHandle<v8::String>(
          isolate_->factory()->InternalizeUtf8String(node->name_)),
      script_name, node->script_id_, node->script_position_, line, column,
      node->id_, std::vector<v8::AllocationProfile::Node*>(), allocations});
  
  v8::AllocationProfile::Node* current = &profile->nodes_.back();
  
  // Process children
  current->children.reserve(node->children_.size());
  for (const auto& it : node->children_) {
    current->children.push_back(
        TranslateAllocationNode(profile, it.second.get(), scripts));
  }
  
  node->pinned_ = false;
  return current;
}