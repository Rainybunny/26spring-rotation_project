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
  // Pre-size level 0 vector since it typically has more files
  if (num_levels > 0) {
    files_[0].reserve(32); // Typical level 0 file count
  }
  // Pre-size other levels
  for (int level = 1; level < num_levels; level++) {
    files_[level].reserve(8); // Typical non-level-0 file count
  }
}