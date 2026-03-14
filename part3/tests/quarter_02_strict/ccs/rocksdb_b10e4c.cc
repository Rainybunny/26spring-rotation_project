Version::Version(VersionSet* vset, uint64_t version_number)
    : vset_(vset), next_(this), prev_(this), refs_(0),
      file_to_compact_(nullptr),
      file_to_compact_level_(-1),
      offset_manifest_file_(0),
      version_number_(version_number) {
  const int num_levels = vset->NumberLevels();
  
  // Combined allocation for level-based arrays
  files_by_size_.resize(num_levels);
  next_file_to_compact_by_size_.resize(num_levels, 0);
  compaction_score_.resize(num_levels);
  compaction_level_.resize(num_levels);
  
  // Allocate files array
  files_ = new std::vector<FileMetaData*>[num_levels];
  
  // Initialize pointers
  for (int i = 0; i < num_levels; i++) {
    compaction_level_[i] = i;
  }
}