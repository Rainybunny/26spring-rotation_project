int strbuf_getwholeline(struct strbuf *sb, FILE *fp, int term)
{
    int ch;
    char buffer[1024];
    char *pos = buffer;
    size_t remaining = sizeof(buffer);

    if (feof(fp))
        return EOF;

    strbuf_reset(sb);
    while ((ch = getc(fp)) != EOF) {
        *pos++ = ch;
        if (--remaining == 0 || ch == term) {
            size_t chunk_size = pos - buffer;
            strbuf_grow(sb, chunk_size);
            memcpy(sb->buf + sb->len, buffer, chunk_size);
            sb->len += chunk_size;
            pos = buffer;
            remaining = sizeof(buffer);
            if (ch == term)
                break;
        }
    }

    if (pos > buffer) {
        size_t remaining_chars = pos - buffer;
        strbuf_grow(sb, remaining_chars);
        memcpy(sb->buf + sb->len, buffer, remaining_chars);
        sb->len += remaining_chars;
    }

    if (ch == EOF && sb->len == 0)
        return EOF;

    sb->buf[sb->len] = '\0';
    return 0;
}