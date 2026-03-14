void *gitmemmem(const void *haystack, size_t haystack_len,
                const void *needle, size_t needle_len)
{
	const char *begin = haystack;
	const char *last_possible = begin + haystack_len - needle_len;
	const char first_char = *(const char *)needle;

	/*
	 * The first occurrence of the empty string is deemed to occur at
	 * the beginning of the string.
	 */
	if (needle_len == 0)
		return (void *)begin;

	/*
	 * Sanity check, otherwise the loop might search through the whole
	 * memory.
	 */
	if (haystack_len < needle_len)
		return NULL;

	for (; begin <= last_possible; begin++) {
		if (*begin == first_char) {
			/* Prefetch next potential match location */
			if (begin + 64 <= last_possible)
				prefetch(begin + 64);
			if (!memcmp(begin, needle, needle_len))
				return (void *)begin;
		}
	}

	return NULL;
}