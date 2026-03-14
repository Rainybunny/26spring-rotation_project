static unsigned int contains(struct diff_filespec *one,
			     const char *needle, unsigned long len,
			     regex_t *regexp)
{
	unsigned int cnt;
	unsigned long sz;
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

	} else { /* Optimized exact string match */
		const char *haystack = data;
		const char *end = haystack + sz;
		const char *p = haystack;
		
		while (p + len <= end) {
			/* Two-way algorithm inspired search */
			const char *a = needle;
			const char *b = p;
			while (a < needle + len && *a == *b) {
				a++;
				b++;
			}
			if (a == needle + len) {
				cnt++;
				p = b; /* Skip past this match */
			} else {
				p++;
			}
		}
	}
	diff_free_filespec_data(one);
	return cnt;
}