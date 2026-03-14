int strbuf_getwholeline(struct strbuf *sb, FILE *fp, int term)
{
    int ch;
    size_t capacity = 128;  // Initial buffer capacity

    if (feof(fp))
        return EOF;

    strbuf_reset(sb);
    strbuf_grow(sb, capacity);

    while ((ch = getc(fp)) != EOF) {
        if (sb->len >= sb->alloc - 1) {
            capacity *= 2;
            strbuf_grow(sb, capacity);
        }
        sb->buf[sb->len++] = ch;
        if (ch == term)
            break;
    }

    if (ch == EOF && sb->len == 0)
        return EOF;

    sb->buf[sb->len] = '\0';
    return 0;
}