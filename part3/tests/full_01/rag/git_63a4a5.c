static void trace_encoding(const char *context, const char *path,
			   const char *encoding, const char *buf, size_t len)
{
	static struct trace_key coe = TRACE_KEY_INIT(WORKING_TREE_ENCODING);
	static const char prefix_fmt[] = "%s (%s, considered %s):\n";
	static const char line_start[] = "| \033[2m";
	static const char line_mid[] = ":\033[0m ";
	static const char line_end[] = "\033[0m";
	struct strbuf trace = STRBUF_INIT;
	int i;

	// Pre-allocate buffer space to reduce reallocations
	strbuf_grow(&trace, len * 8 + 128);
	
	// Add header line
	strbuf_addf(&trace, prefix_fmt, context, path, encoding);

	// Process buffer content
	for (i = 0; i < len && buf; ++i) {
		unsigned char c = buf[i];
		char display = (c > 32 && c < 127) ? c : ' ';
		
		// Add line components directly
		strbuf_add(&trace, line_start, sizeof(line_start)-1);
		strbuf_addf(&trace, "%02i", i);
		strbuf_add(&trace, line_mid, sizeof(line_mid)-1);
		strbuf_addf(&trace, "%02x ", c);
		strbuf_add(&trace, line_end, sizeof(line_end)-1);
		strbuf_addch(&trace, display);
		strbuf_addch(&trace, ((i+1) % 8 && (i+1) < len) ? ' ' : '\n');
	}
	strbuf_addch(&trace, '\n');

	trace_strbuf(&coe, &trace);
	strbuf_release(&trace);
}