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

	} else { /* Boyer-Moore-Horspool string search */
		unsigned long bad_char_shift[256];
		unsigned long last = len - 1;
		unsigned long idx;

		/* Preprocessing */
		for (idx = 0; idx < 256; idx++)
			bad_char_shift[idx] = len;
		for (idx = 0; idx < last; idx++)
			bad_char_shift[(unsigned char)needle[idx]] = last - idx;

		/* Searching */
		offset = 0;
		while (offset <= sz - len) {
			for (idx = last; idx > 0 && data[offset + idx] == needle[idx]; idx--);
			if (idx == 0 && data[offset] == needle[0]) {
				cnt++;
				offset += len;
			} else {
				offset += bad_char_shift[(unsigned char)data[offset + last]];
			}
		}
	}
	diff_free_filespec_data(one);
	return cnt;
}