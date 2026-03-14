void sha1_file_name(struct strbuf *buf, const unsigned char *sha1)
{
	int i;
	const char *objdir = get_object_directory();
	strbuf_reset(buf);
	strbuf_addstr(buf, objdir);
	strbuf_addch(buf, '/');
	for (i = 0; i < 20; i++) {
		static const char hex[] = "0123456789abcdef";
		unsigned int val = sha1[i];
		strbuf_addch(buf, hex[val >> 4]);
		strbuf_addch(buf, hex[val & 0xf]);
		if (!i)
			strbuf_addch(buf, '/');
	}
}