Status Preprocess(HloInstruction* instruction) override {
    // Use a faster hash function optimized for instruction names
    struct StringHash {
      size_t operator()(const std::string& s) const {
        return std::hash<std::string_view>{}(s);
      }
    };
    
    static std::unordered_map<std::string, const HloInstruction*, StringHash> 
        instructions_by_name;
    
    auto previous = instructions_by_name.find(instruction->name());
    TF_RET_CHECK(previous == instructions_by_name.end())
        << "HLO has name that is not unique within module:\n"
        << instruction->ToString()
        << " in computation: " << instruction->parent()->name()
        << "\nPrevious HLO with same name:\n"
        << previous->second->ToString()
        << " in computation: " << previous->second->parent()->name();
    instructions_by_name[instruction->name()] = instruction;

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