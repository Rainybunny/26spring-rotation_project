int strbuf_getwholeline(struct strbuf *sb, FILE *fp, int term)
{
    int ch;
    char buffer[128];
    char *result;

    if (feof(fp))
        return EOF;

    strbuf_reset(sb);
    strbuf_grow(sb, sizeof(buffer));

    while (1) {
        result = fgets(buffer, sizeof(buffer), fp);
        if (!result) {
            if (feof(fp) && sb->len == 0)
                return EOF;
            break;
        }

        size_t len = strlen(buffer);
        strbuf_add(sb, buffer, len);

        if (len > 0 && buffer[len-1] == term)
            break;
        
        if (len < sizeof(buffer) - 1)
            break;
    }

    sb->buf[sb->len] = '\0';
    return 0;
}