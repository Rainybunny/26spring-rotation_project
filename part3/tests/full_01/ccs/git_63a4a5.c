static void trace_encoding(const char *context, const char *path,
			   const char *encoding, const char *buf, size_t len)
{
	static struct trace_key coe = TRACE_KEY_INIT(WORKING_TREE_ENCODING);
	struct strbuf trace = STRBUF_INIT;
	char linebuf[128]; /* Enough for one line of trace */
	int i, pos;

	/* Header line using strbuf since it has variable components */
	strbuf_addf(&trace, "%s (%s, considered %s):\n", context, path, encoding);

	if (!buf)
		goto finish;

	/* Process each byte with direct buffer writes */
	for (i = 0; i < len; ++i) {
		unsigned char c = buf[i];
		pos = snprintf(linebuf, sizeof(linebuf),
			      "| \033[2m%2i:\033[0m %02x \033[2m%c\033[0m%c",
			      i, c, (c > 32 && c < 127 ? c : ' '),
			      ((i+1) % 8 && (i+1) < len ? ' ' : '\n'));
		strbuf_add(&trace, linebuf, pos);
	}

	strbuf_addch(&trace, '\n');

finish:
	trace_strbuf(&coe, &trace);
	strbuf_release(&trace);
}