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

	} else { /* Classic exact string match */
		/* Fast path for short needles */
		if (len <= sizeof(unsigned long)) {
			unsigned long pattern = 0;
			unsigned long word;
			memcpy(&pattern, needle, len);
			
			for (offset = 0; offset + len <= sz; offset++) {
				memcpy(&word, data + offset, len);
				if (word == pattern) {
					offset += len - 1;
					cnt++;
				}
			}
		} else {
			/* Original byte-by-byte comparison for longer needles */
			for (offset = 0; offset + len <= sz; offset++) {
				if (!memcmp(needle, data + offset, len)) {
					offset += len - 1;
					cnt++;
				}
			}
		}
	}
	diff_free_filespec_data(one);
	return cnt;
}