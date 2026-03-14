int strbuf_getwholeline(struct strbuf *sb, FILE *fp, int term)
{
    int ch;
    size_t initial_size = 128;  // Reasonable initial buffer size

    if (feof(fp))
        return EOF;

    strbuf_reset(sb);
    strbuf_grow(sb, initial_size);

    while ((ch = fgetc(fp)) != EOF) {
        if (sb->len + 1 >= sb->alloc)
            strbuf_grow(sb, sb->alloc * 2);  // Double size when needed
        sb->buf[sb->len++] = ch;
        if (ch == term)
            break;
    }

    if (ch == EOF && sb->len == 0)
        return EOF;

    sb->buf[sb->len] = '\0';
    return 0;
}