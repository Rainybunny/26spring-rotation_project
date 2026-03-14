struct option *parse_options_concat(struct option *a, struct option *b)
{
	struct option *ret;
	size_t i = 0, j = 0;

	/* Count and copy 'a' in single pass */
	while (a[i].type != OPTION_END) {
		i++;
	}
	ALLOC_ARRAY(ret, st_add3(i, 0, 1)); /* temp alloc for a + END */
	while (i--) {
		ret[i] = a[i];
	}

	/* Count and copy 'b' in single pass */
	while (b[j].type != OPTION_END) {
		j++;
	}
	REALLOC_ARRAY(ret, st_add3(i, j, 1)); /* resize for a + b + END */
	while (j--) {
		ret[i + j] = b[j];
	}
	ret[i + j] = b[j]; /* final OPTION_END */

	return ret;
}