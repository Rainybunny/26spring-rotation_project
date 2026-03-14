int strbuf_getwholeline(struct strbuf *sb, FILE *fp, int term)
{
    if (feof(fp))
        return EOF;

    strbuf_reset(sb);
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), fp)) {
        char *end = strchr(buffer, term);
        if (end) {
            // Found terminator - copy up to it
            strbuf_add(sb, buffer, end - buffer);
            sb->buf[sb->len] = '\0';
            return 0;
        } else {
            // No terminator - add entire chunk
            strbuf_add(sb, buffer, strlen(buffer));
        }
    }

    if (ferror(fp) || (sb->len == 0 && feof(fp)))
        return EOF;

    sb->buf[sb->len] = '\0';
    return 0;
}