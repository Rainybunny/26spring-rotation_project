static int same_name(const struct cache_entry *ce, const char *name, int namelen, int icase)
{
	int len = ce_namelen(ce);

	if (!icase)
		return len == namelen && !cache_name_compare(name, namelen, ce->name, len);

	if (len == namelen) {
		if (!cache_name_compare(name, namelen, ce->name, len))
			return 1;
		return slow_same_name(name, namelen, ce->name, len);
	}
	return 0;
}