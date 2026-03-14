Version::Version(VersionSet* vset, uint64_t version_number)
    : vset_(vset), next_(this), prev_(this), refs_(0),
      files_by_size_(vset->NumberLevels()),
      next_file_to_compact_by_size_(vset->NumberLevels()),
      file_to_compact_(nullptr),
      file_to_compact_level_(-1),
      compaction_score_(vset->NumberLevels()),
      compaction_level_(vset->NumberLevels()),
      offset_manifest_file_(0),
      version_number_(version_number) {
  const int num_levels = vset->NumberLevels();
  files_ = new std::vector<FileMetaData*>[num_levels];
  // Pre-allocate capacity for each level's vector
  for (int i = 0; i < num_levels; i++) {
    // Level 0 typically has more files, so give it more capacity
    const size_t initial_capacity = (i == 0) ? 32 : 8;
    files_[i].reserve(initial_capacity);
  }
}