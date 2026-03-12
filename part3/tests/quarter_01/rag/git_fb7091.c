static void rehash_objects(struct packing_data *pdata)
{
	uint32_t i;
	struct object_entry *entry;

	pdata->index_size = closest_pow2(pdata->nr_objects * 3);
	if (pdata->index_size < 1024)
		pdata->index_size = 1024;

	pdata->index = xrealloc(pdata->index, sizeof(uint32_t) * pdata->index_size);
	memset(pdata->index, 0, sizeof(uint32_t) * pdata->index_size);

	entry = pdata->objects;

	for (i = 0; i < pdata->nr_objects; i++) {
		uint32_t hash, mask = pdata->index_size - 1;
		uint32_t ix;

		memcpy(&hash, entry->idx.sha1, sizeof(uint32_t));
		ix = hash & mask;

		while (pdata->index[ix] > 0) {
			ix = (ix + 1) & mask;
		}

		pdata->index[ix] = i + 1;
		entry++;
	}
}