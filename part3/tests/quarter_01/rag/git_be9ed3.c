static int same_name(const struct cache_entry *ce, const char *name, int namelen, int icase)
{
	int len = ce->ce_namelen;  // Direct access instead of function call

	/* Fast path: exact match */
	if (len == namelen && !cache_name_compare(name, namelen, ce->name, len))
		return 1;

	/* If not case-insensitive or lengths don't match, fail */
	if (!icase || len != namelen)
		return 0;

	/* Optimized slow path: case-insensitive comparison */
	const char *name1 = name;
	const char *name2 = ce->name;
	int remaining = namelen;

	while (remaining--) {
		unsigned char c1 = *name1++;
		unsigned char c2 = *name2++;
		if (c1 != c2) {
			c1 = toupper(c1);
			c2 = toupper(c2);
			if (c1 != c2)
				return 0;
		}
	}
	return 1;
}