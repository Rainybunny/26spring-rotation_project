Version::~Version() {
  for (int i = 0; i < vset_->NumberLevels(); ++i) {
    files_[i].~vector<FileMetaData*>();
  }
  ::operator delete[](files_);
}