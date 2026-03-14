void sha1_file_name(struct strbuf *buf, const unsigned char *sha1)
{
	const char *obj_dir = get_object_directory();
	size_t obj_dir_len = strlen(obj_dir);
	
	strbuf_grow(buf, obj_dir_len + 1 + 40 + 2); // dir + '/' + sha1 + '/' + NUL
	strbuf_reset(buf);
	strbuf_add(buf, obj_dir, obj_dir_len);
	strbuf_addch(buf, '/');
	
	for (int i = 0; i < 20; i++) {
		static char hex[] = "0123456789abcdef";
		unsigned int val = sha1[i];
		strbuf_addch(buf, hex[val >> 4]);
		strbuf_addch(buf, hex[val & 0xf]);
		if (!i)
			strbuf_addch(buf, '/');
	}
}