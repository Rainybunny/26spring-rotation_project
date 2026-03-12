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
        
        size_t len = strlen(buffer);
        char *term_pos = memchr(buffer, term, len);
        if (term_pos) {
            len = term_pos - buffer + 1;
            strbuf_add(sb, buffer, len);
            break;
        }
        strbuf_add(sb, buffer, len);
    }
    
    return 0;
}