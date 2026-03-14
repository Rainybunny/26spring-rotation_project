// tile_box is inclusive
  SpatialIndexCursor(Iterator* spatial_iterator, ValueGetter* value_getter,
                     const BoundingBox<uint64_t>& tile_bbox, uint32_t tile_bits)
      : value_getter_(value_getter), valid_(true) {
    // calculate quad keys we'll need to query using ordered set
    std::set<uint64_t> quad_keys;
    const uint64_t x_size = tile_bbox.max_x - tile_bbox.min_x + 1;
    const uint64_t y_size = tile_bbox.max_y - tile_bbox.min_y + 1;
    quad_keys.reserve(x_size * y_size);
    
    for (uint64_t x = tile_bbox.min_x; x <= tile_bbox.max_x; ++x) {
      for (uint64_t y = tile_bbox.min_y; y <= tile_bbox.max_y; ++y) {
        quad_keys.insert(GetQuadKeyFromTile(x, y, tile_bits));
      }
    }

    // load primary key ids for all quad keys
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
        primary_key_ids_.insert(id);
        spatial_iterator->Next();
      }
    }

    if (!spatial_iterator->status().ok()) {
      status_ = spatial_iterator->status();
      valid_ = false;
    }
    delete spatial_iterator;

    valid_ = valid_ && primary_key_ids_.size() > 0;

    if (valid_) {
      primary_keys_iterator_ = primary_key_ids_.begin();
      ExtractData();
    }
  }