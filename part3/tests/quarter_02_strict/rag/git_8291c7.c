int strbuf_getwholeline(struct strbuf *sb, FILE *fp, int term)
{
    int ch;
    char buffer[128];
    char *found;

    if (feof(fp))
        return EOF;

    strbuf_reset(sb);
    while (1) {
        if (fgets(buffer, sizeof(buffer), fp) == NULL) {
            if (sb->len == 0)
                return EOF;
            break;
        }

        strbuf_grow(sb, strlen(buffer));
        found = memchr(buffer, term, strlen(buffer));
        if (found) {
            strbuf_add(sb, buffer, found - buffer + 1);
            break;
        } else {
            strbuf_add(sb, buffer, strlen(buffer));
        }
    }

    sb->buf[sb->len] = '\0';
    return 0;
}