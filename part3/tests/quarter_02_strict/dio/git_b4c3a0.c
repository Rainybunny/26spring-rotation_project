int xdl_recmatch(const char *l1, long s1, const char *l2, long s2, long flags)
{
	const char *p1 = l1, *p2 = l2;
	const char *end1 = l1 + s1, *end2 = l2 + s2;

	if (!(flags & XDF_WHITESPACE_FLAGS))
		return s1 == s2 && !memcmp(l1, l2, s1);

	if (flags & XDF_IGNORE_WHITESPACE) {
		goto skip_ws;
		while (p1 < end1 && p2 < end2) {
			if (*p1++ != *p2++)
				return 0;
		skip_ws:
			while (p1 < end1 && isspace(*p1))
				p1++;
			while (p2 < end2 && isspace(*p2))
				p2++;
		}
	} else if (flags & XDF_IGNORE_WHITESPACE_CHANGE) {
		while (p1 < end1 && p2 < end2) {
			if (isspace(*p1) && isspace(*p2)) {
				while (p1 < end1 && isspace(*p1))
					p1++;
				while (p2 < end2 && isspace(*p2))
					p2++;
				continue;
			}
			if (*p1++ != *p2++)
				return 0;
		}
	} else if (flags & XDF_IGNORE_WHITESPACE_AT_EOL) {
		while (p1 < end1 && p2 < end2 && *p1++ == *p2++)
			; /* keep going */
	}

	if (p1 < end1) {
		while (p1 < end1 && isspace(*p1))
			p1++;
		if (end1 != p1)
			return 0;
	}
	if (p2 < end2) {
		while (p2 < end2 && isspace(*p2))
			p2++;
		return (end2 == p2);
	}
	return 1;
}