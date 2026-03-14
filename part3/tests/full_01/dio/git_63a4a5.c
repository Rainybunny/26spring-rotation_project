static void trace_encoding(const char *context, const char *path,
			   const char *encoding, const char *buf, size_t len)
{
	static struct trace_key coe = TRACE_KEY_INIT(WORKING_TREE_ENCODING);
	struct strbuf trace = STRBUF_INIT;
	int i;

	/* Pre-allocate buffer to avoid reallocs */
	strbuf_grow(&trace, 128 + len * 16); /* Conservative estimate */

	strbuf_addf(&trace, "%s (%s, considered %s):\n", context, path, encoding);
	
	if (buf) {
		for (i = 0; i < len; ++i) {
			unsigned char c = buf[i];
			strbuf_addstr(&trace, "| \033[2m");
			if (i < 10) strbuf_addch(&trace, ' ');
			strbuf_addf(&trace, "%d:\033[0m %02x \033[2m", i, c);
			strbuf_addch(&trace, (c > 32 && c < 127) ? c : ' ');
			strbuf_addstr(&trace, "\033[0m");
			strbuf_addch(&trace, ((i+1) % 8 && (i+1) < len) ? ' ' : '\n');
		}
	}
	strbuf_addch(&trace, '\n');

	trace_strbuf(&trace_key_coe, &trace);
	strbuf_release(&trace);
}