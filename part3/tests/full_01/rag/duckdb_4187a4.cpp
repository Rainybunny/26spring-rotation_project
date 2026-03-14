void TupleDataCollection::Scatter(TupleDataChunkState &chunk_state, DataChunk &new_chunk,
                                  const SelectionVector &append_sel, const idx_t append_count) const {
	// Set the validity mask for each row before inserting data
	auto row_locations = FlatVector::GetData<data_ptr_t>(chunk_state.row_locations);
	
	// Optimized path: set all validity bytes at once since we're setting all columns valid
	const auto validity_bytes_size = ValidityBytes::SizeInBytes(layout.ColumnCount());
	if (validity_bytes_size > 0) {
		const uint8_t valid_byte = 0xFF; // All bits set to 1 (valid)
		for (idx_t i = 0; i < append_count; i++) {
			memset(row_locations[i], valid_byte, validity_bytes_size);
		}
	}

	if (!layout.AllConstant()) {
		// Set the heap size for each row
		const auto heap_size_offset = layout.GetHeapSizeOffset();
		const auto heap_sizes = FlatVector::GetData<idx_t>(chunk_state.heap_sizes);
		for (idx_t i = 0; i < append_count; i++) {
			Store<uint32_t>(heap_sizes[i], row_locations[i] + heap_size_offset);
		}
	}

	// Write the data
	for (const auto &col_idx : chunk_state.column_ids) {
		Scatter(chunk_state, new_chunk.data[col_idx], col_idx, append_sel, append_count, new_chunk.size());
	}
}