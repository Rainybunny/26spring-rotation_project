// tile_box is inclusive
  SpatialIndexCursor(Iterator* spatial_iterator, ValueGetter* value_getter,
                     const BoundingBox<uint64_t>& tile_bbox, uint32_t tile_bits)
      : value_getter_(value_getter), valid_(true) {
    // calculate quad keys we'll need to query
    std::vector<uint64_t> quad_keys;
    size_t num_tiles = (tile_bbox.max_x - tile_bbox.min_x + 1) *
                       (tile_bbox.max_y - tile_bbox.min_y + 1);
    quad_keys.reserve(num_tiles);
    for (uint64_t x = tile_bbox.min_x; x <= tile_bbox.max_x; ++x) {
      for (uint64_t y = tile_bbox.min_y; y <= tile_bbox.max_y; ++y) {
        quad_keys.push_back(GetQuadKeyFromTile(x, y, tile_bits));
      }
    }
    std::sort(quad_keys.begin(), quad_keys.end());

    // load primary key ids for all quad keys
    std::vector<uint64_t> primary_key_ids;
    primary_key_ids.reserve(num_tiles * 4); // Estimate 4 items per tile
    
    for (auto quad_key : quad_keys) {
      std::string encoded_quad_key;
      PutFixed64BigEndian(&encoded_quad_key, quad_key);
      Slice slice_quad_key(encoded_quad_key);

      if (!CheckQuadKey(spatial_iterator, slice_quad_key)) {
        spatial_iterator->Seek(slice_quad_key);
      }

      while (CheckQuadKey(spatial_iterator, slice_quad_key)) {
        uint64_t id;
        bool ok = GetFixed64BigEndian(
            Slice(spatial_iterator->key().data() + sizeof(uint64_t),
                  sizeof(uint64_t)),
            &id);
        if (!ok) {
          valid_ = false;
          status_ = Status::Corruption("Spatial index corruption");
          break;
        }
        primary_key_ids.push_back(id);
        spatial_iterator->Next();
      }
    }

    if (!spatial_iterator->status().ok()) {
      status_ = spatial_iterator->status();
      valid_ = false;
    }
    delete spatial_iterator;

    // Remove duplicates by sorting and using unique
    std::sort(primary_key_ids.begin(), primary_key_ids.end());
    auto last = std::unique(primary_key_ids.begin(), primary_key_ids.end());
    primary_key_ids.erase(last, primary_key_ids.end());

    valid_ = valid_ && !primary_key_ids.empty();

    if (valid_) {
      primary_key_ids_ = std::move(primary_key_ids);
      primary_keys_iterator_ = primary_key_ids_.begin();
      ExtractData();
    }
  }