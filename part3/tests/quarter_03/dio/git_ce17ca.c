static unsigned int contains(struct diff_filespec *one,
			     const char *needle, unsigned long len,
			     regex_t *regexp)
{
	unsigned int cnt;
	unsigned long offset, sz;
	const char *data;
	if (diff_populate_filespec(one, 0))
		return 0;
	if (!len)
		return 0;

	sz = one->size;
	data = one->data;
	cnt = 0;

	if (regexp) {
		regmatch_t regmatch;
		int flags = 0;

		while (*data && !regexec(regexp, data, 1, &regmatch, flags)) {
			flags |= REG_NOTBOL;
			data += regmatch.rm_so;
			if (*data) data++;
			cnt++;
		}

	} else { /* Boyer-Moore string search */
		/* Preprocess bad character table */
		unsigned char bad_char[256];
		unsigned long i;
		const unsigned char *needle_bytes = (const unsigned char *)needle;

		for (i = 0; i < 256; i++)
			bad_char[i] = len;
		for (i = 0; i < len - 1; i++)
			bad_char[needle_bytes[i]] = len - 1 - i;

		/* Search */
		offset = 0;
		while (offset + len <= sz) {
			unsigned long j = len - 1;
			while (j != (unsigned long)-1 && data[offset + j] == needle[j])
				j--;

			if (j == (unsigned long)-1) { /* Match found */
				cnt++;
				offset += len;
			} else {
				offset += bad_char[(unsigned char)data[offset + len - 1]];
			}
		}
	}
	diff_free_filespec_data(one);
	return cnt;
}