int xdl_recmatch(const char *l1, long s1, const char *l2, long s2, long flags)
{
	int i1, i2;

	if (!(flags & XDF_WHITESPACE_FLAGS))
		return s1 == s2 && !memcmp(l1, l2, s1);

	i1 = 0;
	i2 = 0;

	if (flags & XDF_IGNORE_WHITESPACE) {
		while (1) {
			/* Skip whitespace in both strings */
			while (i1 < s1 && isspace(l1[i1]))
				i1++;
			while (i2 < s2 && isspace(l2[i2]))
				i2++;
			/* Check if we've reached end of both strings */
			if (i1 >= s1 || i2 >= s2)
				break;
			/* Compare non-whitespace characters */
			if (l1[i1++] != l2[i2++])
				return 0;
		}
	} else if (flags & XDF_IGNORE_WHITESPACE_CHANGE) {
		while (i1 < s1 && i2 < s2) {
			if (isspace(l1[i1]) && isspace(l2[i2])) {
				/* Skip matching spaces and try again */
				while (i1 < s1 && isspace(l1[i1]))
					i1++;
				while (i2 < s2 && isspace(l2[i2]))
					i2++;
				continue;
			}
			if (l1[i1++] != l2[i2++])
				return 0;
		}
	} else if (flags & XDF_IGNORE_WHITESPACE_AT_EOL) {
		while (i1 < s1 && i2 < s2 && l1[i1++] == l2[i2++])
			; /* keep going */
	}

	/* Check remaining characters in both strings */
	if (i1 < s1) {
		while (i1 < s1 && isspace(l1[i1]))
			i1++;
		if (i1 < s1)
			return 0;
	}
	if (i2 < s2) {
		while (i2 < s2 && isspace(l2[i2]))
			i2++;
		return (i2 >= s2);
	}
	return 1;
}