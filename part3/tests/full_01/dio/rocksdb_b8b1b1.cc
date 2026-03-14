Status VersionSet::Recover(
    const std::vector<ColumnFamilyDescriptor>& column_families,
    bool read_only) {
  // ... (rest of initial code remains same until manifest filename handling)

  // Optimized manifest filename handling
  std::string manifest_filename;
  manifest_filename.reserve(256); // Pre-allocate reasonable size
  Status s = ReadFileToString(env_, CurrentFileName(dbname_), &manifest_filename);
  if (!s.ok()) {
    return s;
  }
  
  // Remove trailing newline more efficiently
  if (manifest_filename.empty() || manifest_filename.back() != '\n') {
    return Status::Corruption("CURRENT file does not end with newline");
  }
  manifest_filename.pop_back(); // More efficient than resize()

  FileType type;
  bool parse_ok = ParseFileName(manifest_filename, &manifest_file_number_, &type);
  if (!parse_ok || type != kDescriptorFile) {
    return Status::Corruption("CURRENT file corrupted");
  }

  Log(db_options_->info_log, "Recovering from manifest file: %s\n",
      manifest_filename.c_str());

  // Build full path more efficiently
  std::string full_manifest_path;
  full_manifest_path.reserve(dbname_.size() + 1 + manifest_filename.size());
  full_manifest_path.append(dbname_);
  full_manifest_path.push_back('/');
  full_manifest_path.append(manifest_filename);

  unique_ptr<SequentialFile> manifest_file;
  s = env_->NewSequentialFile(full_manifest_path, &manifest_file, env_options_);
  if (!s.ok()) {
    return s;
  }

  uint64_t manifest_file_size;
  s = env_->GetFileSize(full_manifest_path, &manifest_file_size);
  if (!s.ok()) {
    return s;
  }

  // ... (rest of function remains same)
  
  return s;
}