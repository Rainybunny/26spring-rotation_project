void sha1_file_name(struct strbuf *buf, const unsigned char *sha1)
{
	static const char *object_dir = NULL;
	static struct strbuf fmt = STRBUF_INIT;
	
	if (!object_dir) {
		object_dir = get_object_directory();
		strbuf_addf(&fmt, "%s/", object_dir);
	}
	
	strbuf_reset(buf);
	strbuf_add(buf, fmt.buf, fmt.len);
	fill_sha1_path(buf, sha1);
}