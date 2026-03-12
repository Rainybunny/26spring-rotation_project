static inline void strbuf_addch(struct strbuf *sb, int c)
{
	if (sb->len >= (sb->alloc ? sb->alloc - 1 : 0))
		strbuf_grow(sb, 1);
	sb->buf[sb->len++] = c;
	sb->buf[sb->len] = '\0';
}