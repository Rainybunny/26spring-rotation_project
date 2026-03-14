absl::StatusOr<bool> MergeDots(HloComputation* comp,
                               int64_t max_size_to_merge) {
  auto is_merge_candidate = [&](HloInstruction* instr) {
    int64_t bytes = ShapeUtil::ByteSizeOfElements(instr->shape());
    for (const HloInstruction* operand : instr->operands()) {
      bytes += ShapeUtil::ByteSizeOfElements(operand->shape());
    }
    return bytes <= max_size_to_merge;
  };

  // Collect equivalence classes.
  absl::flat_hash_map<HloInstruction*, absl::flat_hash_set<HloInstruction*>>
      equivalence_classes;
  for (HloInstruction* instr : comp->instructions()) {
    if (instr->opcode() != HloOpcode::kDot ||
        !instr->control_predecessors().empty() ||
        !instr->control_successors().empty()) {
      continue;
    }
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

  // Prepare for merging
  absl::flat_hash_set<HloInstruction*> dead_instrs;
  dead_instrs.reserve(equivalence_classes.size() * 2);  // Pre-allocate space
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
      if (a == nullptr) continue;

      for (int64_t j = i + 1; j < dots.size(); j++) {
        HloInstruction* b = dots[j];
        if (b == nullptr) continue;

        // Combined dead check for both instructions
        if (dead_instrs.contains(a) || dead_instrs.contains(b)) continue;

        int32_t a_id = graph_id(a);
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
          
          // Batch successor updates
          auto a_succ = graph.SuccessorsCopy(a_id);
          auto b_succ = graph.SuccessorsCopy(b_id);
          for (int32_t succ : a_succ) graph.InsertEdge(merged_id, succ);
          for (int32_t succ : b_succ) graph.InsertEdge(merged_id, succ);

          dead_instrs.insert(a);
          dead_instrs.insert(b);
          dots[i] = merged;
          dots[j] = nullptr;
        }
      }
    }
  }

  for (HloInstruction* instr : dead_instrs) {
    TF_RETURN_IF_ERROR(comp->RemoveInstruction(instr));
  }

  return !dead_instrs.empty();
}