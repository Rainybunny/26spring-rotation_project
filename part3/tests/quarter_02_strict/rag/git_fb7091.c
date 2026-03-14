static void rehash_objects(struct packing_data *pdata)
{
	uint32_t i;
	struct object_entry *entry;
	uint32_t new_size = closest_pow2(pdata->nr_objects * 3);

	if (new_size < 1024)
		new_size = 1024;

	/* Use xcalloc to get zeroed memory in one operation */
	uint32_t *new_index = xcalloc(new_size, sizeof(uint32_t));
	free(pdata->index);
	pdata->index = new_index;
	pdata->index_size = new_size;

	entry = pdata->objects;

	for (i = 0; i < pdata->nr_objects; i++) {
		int found;
		uint32_t ix = locate_object_entry_hash(pdata, entry->idx.sha1, &found);

		if (found)
			die("BUG: Duplicate object in hash");

		pdata->index[ix] = i + 1;
		entry++;
	}
}