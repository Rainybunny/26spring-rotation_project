Status Preprocess(HloInstruction* instruction) override {
  // Optimized name check using try_emplace
  auto [it, inserted] = instructions_by_name_.try_emplace(instruction->name(), instruction);
  if (!inserted) {
    return InternalError(
        "HLO has name that is not unique within module:\n"
        "%s in computation: %s\nPrevious HLO with same name:\n%s in computation: %s",
        instruction->ToString(), instruction->parent()->name(),
        it->second->ToString(), it->second->parent()->name());
  }

  // Conditional sharding validation
  if (instruction->has_sharding()) {
    Status status =
        instruction->sharding().Validate(instruction->shape(), num_devices_);
    if (!status.ok()) {
      return Status(
          status.code(),
          absl::StrCat("Invalid sharding for instruction: ",
                       instruction->ToString(), ": ", status.error_message()));
    }
  }

  return OkStatus();
}