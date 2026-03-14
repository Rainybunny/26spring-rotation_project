StatusOr<bool> HloCSE::Run(HloModule* module) {
  static const auto eq_instructions = std::equal_to<const HloInstruction*>();
  static const auto eq_computations = [](const HloComputation* lhs,
                                        const HloComputation* rhs) {
    return *lhs == *rhs;
  };
  static const auto cse_equal = [](const HloInstruction* lhs,
                                  const HloInstruction* rhs) {
    return lhs->Identical(*rhs, eq_instructions, eq_computations,
                         is_layout_sensitive_);
  };

  bool changed = false;
  for (auto* computation : module->computations()) {
    if (only_fusion_computations_ && !computation->IsFusionComputation()) {
      continue;
    }

    TF_ASSIGN_OR_RETURN(bool combined,
                       CombineConstants(computation, is_layout_sensitive_));
    changed |= combined;

    absl::flat_hash_set<HloInstruction*, decltype(&CseHash),
                       decltype(cse_equal)>
        representatives(/*N=*/computation->instruction_count() + 1, &CseHash,
                       cse_equal);
    for (auto instruction : computation->MakeInstructionPostOrder()) {
      if (instruction->operand_count() == 0 &&
          instruction->opcode() != HloOpcode::kPartitionId &&
          instruction->opcode() != HloOpcode::kReplicaId) {
        continue;
      }
      if (instruction->HasSideEffect()) {
        continue;
      }

      auto it = representatives.find(instruction);
      if (it != representatives.end()) {
        HloInstruction* equivalent_instruction = *it;
        TF_RETURN_IF_ERROR(
            instruction->ReplaceAllUsesWith(equivalent_instruction));
        TF_RETURN_IF_ERROR(computation->RemoveInstruction(instruction));
        changed = true;
        continue;
      }
      representatives.insert(instruction);
    }
  }
  return changed;
}