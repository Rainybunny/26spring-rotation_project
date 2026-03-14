static int same_name(const struct cache_entry *ce, const char *name, int namelen, int icase)
{
	int len = ce_namelen(ce);
	const char *ce_name = ce->name;

	/*
	 * Always do exact compare, even if we want a case-ignoring comparison;
	 * we do the quick exact one first, because it will be the common case.
	 */
	if (len == namelen) {
		const char *p1 = name;
		const char *p2 = ce_name;
		while (namelen--) {
			if (*p1++ != *p2++)
				goto slow_path;
		}
		return 1;
	}

	if (!icase)
		return 0;

slow_path:
	return slow_same_name(name, namelen, ce_name, len);
}