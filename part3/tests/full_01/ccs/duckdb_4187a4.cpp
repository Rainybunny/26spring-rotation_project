void TupleDataCollection::Scatter(TupleDataChunkState &chunk_state, DataChunk &new_chunk,
                                  const SelectionVector &append_sel, const idx_t append_count) const {
	auto row_locations = FlatVector::GetData<data_ptr_t>(chunk_state.row_locations);
	const auto column_count = layout.ColumnCount();
	const auto validity_mask_size = ValidityBytes::ValidityMaskSize(column_count);
	const auto validity_mask = ValidityBytes::AllValid(column_count);

	if (layout.AllConstant()) {
		// Only need to set validity masks
		for (idx_t i = 0; i < append_count; i++) {
			memcpy(row_locations[i], validity_mask, validity_mask_size);
		}
	} else {
		// Set both validity masks and heap sizes
		const auto heap_size_offset = layout.GetHeapSizeOffset();
		const auto heap_sizes = FlatVector::GetData<idx_t>(chunk_state.heap_sizes);
		for (idx_t i = 0; i < append_count; i++) {
			auto row_location = row_locations[i];
			memcpy(row_location, validity_mask, validity_mask_size);
			Store<uint32_t>(heap_sizes[i], row_location + heap_size_offset);
		}
	}

	// Write the data
	for (const auto &col_idx : chunk_state.column_ids) {
		Scatter(chunk_state, new_chunk.data[col_idx], col_idx, append_sel, append_count, new_chunk.size());
	}
}