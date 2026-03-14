absl::StatusOr<bool> MergeDots(HloComputation* comp,
                               int64_t max_size_to_merge) {
  // Pre-compute merge candidacy to avoid repeated calculations
  absl::flat_hash_map<const HloInstruction*, bool> is_candidate_cache;
  auto is_merge_candidate = [&](HloInstruction* instr) {
    auto it = is_candidate_cache.find(instr);
    if (it != is_candidate_cache.end()) return it->second;
    
    int64_t bytes = ShapeUtil::ByteSizeOfElements(instr->shape());
    for (const HloInstruction* operand : instr->operands()) {
      bytes += ShapeUtil::ByteSizeOfElements(operand->shape());
    }
    bool result = bytes <= max_size_to_merge;
    is_candidate_cache[instr] = result;
    return result;
  };

  // Build equivalence classes with early filtering
  absl::flat_hash_map<HloInstruction*, absl::flat_hash_set<HloInstruction*>>
      equivalence_classes;
  for (HloInstruction* instr : comp->instructions()) {
    // Early filtering of non-dot and control-dependent instructions
    if (instr->opcode() != HloOpcode::kDot ||
        !instr->control_predecessors().empty() ||
        !instr->control_successors().empty()) {
      continue;
    }
    
    // Only consider candidates or instructions that share operands with candidates
    bool has_candidate = false;
    for (HloInstruction* operand : instr->operands()) {
      if (is_merge_candidate(instr)) {
        has_candidate = true;
        break;
      }
    }
    if (!has_candidate) continue;

    for (HloInstruction* operand : instr->operands()) {
      equivalence_classes[operand].insert(instr);
    }
  }

  // Remove uninteresting equivalence classes
  absl::erase_if(
      equivalence_classes,
      [&](const std::pair<const HloInstruction*,
                          absl::flat_hash_set<HloInstruction*>>& kv) {
        const auto& v = kv.second;
        return v.size() < 2 || absl::c_none_of(v, is_merge_candidate);
      });

  if (equivalence_classes.empty()) {
    return false;
  }

  // Build dependency graph
  tensorflow::GraphCycles graph;
  absl::flat_hash_map<HloInstruction*, int32_t> graph_ids_map;
  auto graph_id = [&](HloInstruction* instr) {
    auto it_and_inserted = graph_ids_map.emplace(instr, -1);
    if (it_and_inserted.second) {
      it_and_inserted.first->second = graph.NewNode();
    }
    return it_and_inserted.first->second;
  };

  for (HloInstruction* instr : comp->MakeInstructionPostOrder()) {
    int32_t id = graph_id(instr);
    for (HloInstruction* operand : instr->operands()) {
      CHECK(graph.InsertEdge(graph_id(operand), id));
    }
    for (HloInstruction* control_pred : instr->control_predecessors()) {
      CHECK(graph.InsertEdge(graph_id(control_pred), id));
    }
  }

  // Merge within equivalence classes
  absl::flat_hash_set<HloInstruction*> dead_instrs;
  std::vector<HloInstruction*> keys;
  keys.reserve(equivalence_classes.size());
  for (auto& kv : equivalence_classes) {
    keys.push_back(kv.first);
  }
  absl::c_sort(keys, [](const HloInstruction* a, const HloInstruction* b) {
    return a->unique_id() < b->unique_id();
  });

  for (auto key : keys) {
    const auto& values = equivalence_classes[key];
    absl::InlinedVector<HloInstruction*, 16> dots(values.begin(), values.end());
    absl::c_sort(dots, [](const HloInstruction* a, const HloInstruction* b) {
      return a->unique_id() < b->unique_id();
    });

    for (int64_t i = 0; i < dots.size(); i++) {
      HloInstruction*& a = dots[i];
      if (a == nullptr || dead_instrs.contains(a)) continue;
      
      int32_t a_id = graph_id(a);
      for (int64_t j = i + 1; j < dots.size(); j++) {
        HloInstruction* b = dots[j];
        if (b == nullptr || dead_instrs.contains(b)) continue;
        
        int32_t b_id = graph_id(b);
        if (graph.IsReachableNonConst(a_id, b_id) ||
            graph.IsReachableNonConst(b_id, a_id) ||
            (!is_merge_candidate(a) && !is_merge_candidate(b))) {
          continue;
        }

        TF_ASSIGN_OR_RETURN(HloInstruction * merged, TryMergeSameOperand(a, b));
        if (merged != nullptr) {
          int32_t merged_id = graph_id(merged);
          graph.InsertEdge(a_id, merged_id);
          graph.InsertEdge(b_id, merged_id);
          for (int32_t succ : graph.SuccessorsCopy(a_id)) {
            graph.InsertEdge(merged_id, succ);
          }
          for (int32_t succ : graph.SuccessorsCopy(b_id)) {
            graph.InsertEdge(merged_id, succ);
          }

          dead_instrs.insert(a);
          dead_instrs.insert(b);
          dots[i] = merged;
          dots[j] = nullptr;
          break;  // Move to next a after successful merge
        }
      }
    }
  }

  for (HloInstruction* instr : dead_instrs) {
    TF_RETURN_IF_ERROR(comp->RemoveInstruction(instr));
  }

  return !dead_instrs.empty();
}