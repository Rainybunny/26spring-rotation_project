Status Preprocess(HloInstruction* instruction) override {
    // Reserve space upfront to avoid rehashing
    if (instructions_by_name_.empty() && instruction->parent() != nullptr) {
      instructions_by_name_.reserve(instruction->parent()->instruction_count());
    }

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