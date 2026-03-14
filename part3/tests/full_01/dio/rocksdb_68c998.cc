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
    unique_ptr<RandomAccessFile> file;
    Status s = Env::Default()->NewRandomAccessFile(fname, &file, EnvOptions());
    ASSERT_TRUE(s.ok()) << s.ToString();

    unique_ptr<WritableFile> outfile;
    s = Env::Default()->ReopenWritableFile(fname, &outfile, EnvOptions());
    ASSERT_TRUE(s.ok()) << s.ToString();

    char* buf = new char[bytes_to_corrupt];
    Slice result;
    s = file->Read(offset, bytes_to_corrupt, &result, buf);
    ASSERT_TRUE(s.ok()) << s.ToString();

    for (int i = 0; i < bytes_to_corrupt; i++) {
      buf[i] ^= 0x80;
    }

    s = outfile->Write(offset, Slice(buf, bytes_to_corrupt));
    ASSERT_TRUE(s.ok()) << s.ToString();
    delete[] buf;
  }