void CorruptFile(const std::string fname, int offset, int bytes_to_corrupt) {
    struct stat sbuf;
    if (stat(fname.c_str(), &sbuf) != 0) {
      const char* msg = strerror(errno);
      ASSERT_TRUE(false) << fname << ": " << msg;
    }

    if (offset < 0) {
      // Relative to end of file; make it absolute
      if (-offset > sbuf.st_size) {
        offset = 0;
      } else {
        offset = sbuf.st_size + offset;
      }
    }
    if (offset > sbuf.st_size) {
      offset = sbuf.st_size;
    }
    if (offset + bytes_to_corrupt > sbuf.st_size) {
      bytes_to_corrupt = sbuf.st_size - offset;
    }

    // Do it
    int fd = open(fname.c_str(), O_RDWR);
    ASSERT_TRUE(fd >= 0) << "Failed to open file: " << fname;

    // Read the portion to corrupt
    std::unique_ptr<char[]> buf(new char[bytes_to_corrupt]);
    ssize_t bytes_read = pread(fd, buf.get(), bytes_to_corrupt, offset);
    ASSERT_TRUE(bytes_read == bytes_to_corrupt) << "Failed to read file portion";

    // Corrupt the bytes
    for (int i = 0; i < bytes_to_corrupt; i++) {
      buf[i] ^= 0x80;
    }

    // Write back the corrupted portion
    ssize_t bytes_written = pwrite(fd, buf.get(), bytes_to_corrupt, offset);
    ASSERT_TRUE(bytes_written == bytes_to_corrupt) << "Failed to write corrupted data";

    close(fd);
}