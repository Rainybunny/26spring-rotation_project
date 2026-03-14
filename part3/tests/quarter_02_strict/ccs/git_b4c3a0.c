int xdl_recmatch(const char *l1, long s1, const char *l2, long s2, long flags)
{
	const char *cur1 = l1, *end1 = l1 + s1;
	const char *cur2 = l2, *end2 = l2 + s2;

	if (!(flags & XDF_WHITESPACE_FLAGS))
		return s1 == s2 && !memcmp(l1, l2, s1);

	/*
	 * -w matches everything that matches with -b, and -b in turn
	 * matches everything that matches with --ignore-space-at-eol.
	 *
	 * Each flavor of ignoring needs different logic to skip whitespaces
	 * while we have both sides to compare.
	 */
	if (flags & XDF_IGNORE_WHITESPACE) {
		goto skip_ws;
		while (cur1 < end1 && cur2 < end2) {
			if (*cur1++ != *cur2++)
				return 0;
		skip_ws:
			while (cur1 < end1 && isspace(*cur1))
				cur1++;
			while (cur2 < end2 && isspace(*cur2))
				cur2++;
		}
	} else if (flags & XDF_IGNORE_WHITESPACE_CHANGE) {
		while (cur1 < end1 && cur2 < end2) {
			char c1 = *cur1, c2 = *cur2;
			if (isspace(c1) && isspace(c2)) {
				/* Skip matching spaces and try again */
				while (cur1 < end1 && isspace(*cur1))
					cur1++;
				while (cur2 < end2 && isspace(*cur2))
					cur2++;
				continue;
			}
			if (c1 != c2)
				return 0;
			cur1++;
			cur2++;
		}
	} else if (flags & XDF_IGNORE_WHITESPACE_AT_EOL) {
		while (cur1 < end1 && cur2 < end2 && *cur1++ == *cur2++)
			; /* keep going */
	}

	/*
	 * After running out of one side, the remaining side must have
	 * nothing but whitespace for the lines to match.  Note that
	 * ignore-whitespace-at-eol case may break out of the loop
	 * while there still are characters remaining on both lines.
	 */
	if (cur1 < end1) {
		while (cur1 < end1 && isspace(*cur1))
			cur1++;
		if (end1 != cur1)
			return 0;
	}
	if (cur2 < end2) {
		while (cur2 < end2 && isspace(*cur2))
			cur2++;
		return (end2 == cur2);
	}
	return 1;
}