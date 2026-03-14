void CommonWebContentsDelegate::DevToolsRequestFileSystems() {
  content::WebContents* devtools_web_contents = GetDevToolsWebContents();
  auto file_system_paths = GetAddedFileSystemPaths(devtools_web_contents);
  
  base::ListValue file_system_value;
  if (!file_system_paths.empty()) {
    for (auto file_system_path : file_system_paths) {
      base::FilePath path = base::FilePath::FromUTF8Unsafe(file_system_path);
      std::string file_system_id = RegisterFileSystem(devtools_web_contents, path);
      FileSystem file_system = CreateFileSystemStruct(devtools_web_contents,
                                                     file_system_id,
                                                     file_system_path);
      file_system_value.Append(CreateFileSystemValue(file_system));
    }
  }

  web_contents_->CallClientFunction("DevToolsAPI.fileSystemsLoaded",
                                   &file_system_value, nullptr, nullptr);
}