Version::Version(VersionSet* vset, uint64_t version_number)
    : vset_(vset), next_(this), prev_(this), refs_(0),
      file_to_compact_(nullptr),
      file_to_compact_level_(-1),
      offset_manifest_file_(0),
      version_number_(version_number) {
  const int num_levels = vset->NumberLevels();
  
  // Use resize instead of constructor initialization
  files_by_size_.resize(num_levels);
  next_file_to_compact_by_size_.resize(num_levels);
  compaction_score_.resize(num_levels);
  compaction_level_.resize(num_levels);
  
  // Initialize all next_file_to_compact_by_size_ to 0
  std::fill(next_file_to_compact_by_size_.begin(), 
            next_file_to_compact_by_size_.end(), 0);
            
  // Allocate files array
  files_ = new std::vector<FileMetaData*>[num_levels];
}