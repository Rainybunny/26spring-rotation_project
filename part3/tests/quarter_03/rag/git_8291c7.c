int strbuf_getwholeline(struct strbuf *sb, FILE *fp, int term)
{
    if (feof(fp))
        return EOF;

    strbuf_reset(sb);
    
    char buffer[1024];
    size_t bytes_read;
    char *found;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        strbuf_grow(sb, bytes_read);
        found = memchr(buffer, term, bytes_read);
        
        if (found) {
            size_t len = found - buffer + 1;
            strbuf_add(sb, buffer, len);
            sb->buf[sb->len] = '\0';
            return 0;
        }
        
        strbuf_add(sb, buffer, bytes_read);
    }

    if (ferror(fp) || (sb->len == 0 && feof(fp)))
        return EOF;

    sb->buf[sb->len] = '\0';
    return 0;
}