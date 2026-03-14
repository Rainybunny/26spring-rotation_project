int strbuf_getwholeline(struct strbuf *sb, FILE *fp, int term)
{
    if (feof(fp))
        return EOF;

    strbuf_reset(sb);
    
    while (1) {
        char buffer[4096];
        size_t nread = fread(buffer, 1, sizeof(buffer), fp);
        
        if (nread == 0) {
            if (feof(fp)) {
                if (sb->len == 0)
                    return EOF;
                break;
            }
            return EOF;
        }

        char *end = memchr(buffer, term, nread);
        if (end) {
            size_t len = end - buffer + 1;
            strbuf_grow(sb, len);
            memcpy(sb->buf + sb->len, buffer, len);
            sb->len += len;
            sb->buf[sb->len] = '\0';
            return 0;
        }

        strbuf_grow(sb, nread);
        memcpy(sb->buf + sb->len, buffer, nread);
        sb->len += nread;
    }

    sb->buf[sb->len] = '\0';
    return 0;
}