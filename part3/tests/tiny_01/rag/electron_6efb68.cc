void CommonWebContentsDelegate::DevToolsRequestFileSystems() {
  auto* devtools_web_contents = GetDevToolsWebContents();
  auto file_system_paths = GetAddedFileSystemPaths(devtools_web_contents);
  if (file_system_paths.empty()) {
    base::ListValue empty_file_system_value;
    web_contents_->CallClientFunction("DevToolsAPI.fileSystemsLoaded",
                                      &empty_file_system_value,
                                      nullptr, nullptr);
    return;
  }

  std::vector<FileSystem> file_systems;
  for (auto file_system_path : file_system_paths) {
    base::FilePath path = base::FilePath::FromUTF8Unsafe(file_system_path);
    std::string file_system_id = RegisterFileSystem(devtools_web_contents,
                                                    path);
    FileSystem file_system = CreateFileSystemStruct(devtools_web_contents,
                                                    file_system_id,
                                                    file_system_path);
    file_systems.push_back(file_system);
  }

  base::ListValue file_system_value;
  for (const auto& file_system : file_systems)
    file_system_value.Append(CreateFileSystemValue(file_system));
  web_contents_->CallClientFunction("DevToolsAPI.fileSystemsLoaded",
                                    &file_system_value, nullptr, nullptr);
}