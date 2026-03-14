Status Preprocess(HloInstruction* instruction) override {
  if (!opts_.verify_unique_names) {
    // Skip name verification if not required
    if (instruction->has_sharding()) {
      Status status =
          instruction->sharding().Validate(instruction->shape(), num_devices_);
      if (!status.ok()) {
        return Status(status.code(),
                      absl::StrCat("Invalid sharding for instruction: ",
                                   instruction->ToString(), ": ",
                                   status.error_message()));
      }
    }
    return OkStatus();
  }

  // Original verification code when name checking is required
  auto previous = instructions_by_name_.find(instruction->name());
  TF_RET_CHECK(previous == instructions_by_name_.end())
      << "HLO has name that is not unique within module:\n"
      << instruction->ToString()
      << " in computation: " << instruction->parent()->name()
      << "\nPrevious HLO with same name:\n"
      << previous->second->ToString()
      << " in computation: " << previous->second->parent()->name();
  instructions_by_name_[instruction->name()] = instruction;

  if (instruction->has_sharding()) {
    Status status =
        instruction->sharding().Validate(instruction->shape(), num_devices_);
    if (!status.ok()) {
      return Status(status.code(),
                    absl::StrCat("Invalid sharding for instruction: ",
                                 instruction->ToString(), ": ",
                                 status.error_message()));
    }
  }

  return OkStatus();
}