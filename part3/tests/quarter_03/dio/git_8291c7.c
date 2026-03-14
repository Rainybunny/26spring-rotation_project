int strbuf_getwholeline(struct strbuf *sb, FILE *fp, int term)
{
    if (feof(fp))
        return EOF;

    strbuf_reset(sb);
    
    char buffer[1024];
    while (1) {
        char *result = fgets(buffer, sizeof(buffer), fp);
        if (!result) {
            if (sb->len == 0)
                return EOF;
            break;
        }

        char *term_pos = strchr(buffer, term);
        if (term_pos) {
            strbuf_add(sb, buffer, term_pos - buffer + 1);
            break;
        } else {
            strbuf_add(sb, buffer, strlen(buffer));
        }
    }

    sb->buf[sb->len] = '\0';
    return 0;
}