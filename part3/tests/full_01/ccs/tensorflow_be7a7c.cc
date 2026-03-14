Status Preprocess(HloInstruction* instruction) override {
    const std::string& name = instruction->name();
    auto previous = instructions_by_name_.find(name);
    if (previous != instructions_by_name_.end()) {
        return Status(
            absl::StatusCode::kFailedPrecondition,
            absl::StrCat("HLO has name that is not unique within module:\n",
                        instruction->ToString(),
                        " in computation: ", instruction->parent()->name(),
                        "\nPrevious HLO with same name:\n",
                        previous->second->ToString(),
                        " in computation: ", previous->second->parent()->name()));
    }

    instructions_by_name_[name] = instruction;

    if (instruction->has_sharding()) {
        Status status =
            instruction->sharding().Validate(instruction->shape(), num_devices_);
        if (!status.ok()) {
            return Status(
                status.code(),
                absl::StrCat("Invalid sharding for instruction: ",
                            instruction->ToString(), ": ",
                            status.error_message()));
        }
    }

    return OkStatus();
}