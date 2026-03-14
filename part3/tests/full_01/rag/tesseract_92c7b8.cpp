void UnicharAmbigs::LoadUnicharAmbigs(const UNICHARSET &encoder_set, TFile *ambig_file,
                                      int debug_level, bool use_ambigs_for_adaption,
                                      UNICHARSET *unicharset) {
  int i, j;
  UnicharIdVector *adaption_ambigs_entry;
  if (debug_level) {
    tprintf("Reading ambiguities\n");
  }

  int test_ambig_part_size;
  int replacement_ambig_part_size;
  const int kBufferSize = 10 + 2 * kMaxAmbigStringSize;
  char *buffer = new char[kBufferSize];
  char replacement_string[kMaxAmbigStringSize];
  UNICHAR_ID test_unichar_ids[MAX_AMBIG_SIZE + 1];
  int line_num = 0;
  int type = NOT_AMBIG;

  // Determine the version of the ambigs file.
  int version = 0;
  ASSERT_HOST(ambig_file->FGets(buffer, kBufferSize) != nullptr && strlen(buffer) > 0);
  if (*buffer == 'v') {
    version = static_cast<int>(strtol(buffer + 1, nullptr, 10));
    ++line_num;
  } else {
    ambig_file->Rewind();
  }
  while (ambig_file->FGets(buffer, kBufferSize) != nullptr) {
    chomp_string(buffer);
    if (debug_level > 2) {
      tprintf("read line %s\n", buffer);
    }
    ++line_num;
    if (!ParseAmbiguityLine(line_num, version, debug_level, encoder_set, buffer,
                            &test_ambig_part_size, test_unichar_ids, &replacement_ambig_part_size,
                            replacement_string, &type)) {
      continue;
    }
    auto *ambig_spec = new AmbigSpec();
    if (!InsertIntoTable((type == REPLACE_AMBIG) ? replace_ambigs_ : dang_ambigs_,
                         test_ambig_part_size, test_unichar_ids, replacement_ambig_part_size,
                         replacement_string, type, ambig_spec, unicharset)) {
      continue;
    }

    if (test_ambig_part_size == 1 && replacement_ambig_part_size == 1 && type == DEFINITE_AMBIG) {
      if (one_to_one_definite_ambigs_[test_unichar_ids[0]] == nullptr) {
        one_to_one_definite_ambigs_[test_unichar_ids[0]] = new UnicharIdVector();
      }
      one_to_one_definite_ambigs_[test_unichar_ids[0]]->push_back(ambig_spec->correct_ngram_id);
    }
    
    if (use_ambigs_for_adaption) {
      std::vector<UNICHAR_ID> encoding;
      if (unicharset->encode_string(replacement_string, true, &encoding, nullptr, nullptr)) {
        for (i = 0; i < test_ambig_part_size; ++i) {
          if (ambigs_for_adaption_[test_unichar_ids[i]] == nullptr) {
            ambigs_for_adaption_[test_unichar_ids[i]] = new UnicharIdVector();
          }
          adaption_ambigs_entry = ambigs_for_adaption_[test_unichar_ids[i]];
          for (int id_to_insert : encoding) {
            ASSERT_HOST(id_to_insert != INVALID_UNICHAR_ID);
            // Use binary search to find insertion point
            auto it = std::lower_bound(adaption_ambigs_entry->begin(),
                                      adaption_ambigs_entry->end(),
                                      id_to_insert,
                                      std::greater<UNICHAR_ID>());
            if (it == adaption_ambigs_entry->end() || *it != id_to_insert) {
              adaption_ambigs_entry->insert(it, id_to_insert);
            }
          }
        }
      }
    }
  }
  delete[] buffer;

  if (use_ambigs_for_adaption) {
    for (i = 0; i < ambigs_for_adaption_.size(); ++i) {
      adaption_ambigs_entry = ambigs_for_adaption_[i];
      if (adaption_ambigs_entry == nullptr) continue;
      for (j = 0; j < adaption_ambigs_entry->size(); ++j) {
        UNICHAR_ID ambig_id = (*adaption_ambigs_entry)[j];
        if (reverse_ambigs_for_adaption_[ambig_id] == nullptr) {
          reverse_ambigs_for_adaption_[ambig_id] = new UnicharIdVector();
        }
        reverse_ambigs_for_adaption_[ambig_id]->push_back(i);
      }
    }
  }

  if (debug_level > 1) {
    // ... (rest of debug printing code remains unchanged)
  }
}