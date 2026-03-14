int strbuf_getwholeline(struct strbuf *sb, FILE *fp, int term)
{
    if (feof(fp))
        return EOF;

    strbuf_reset(sb);
    
    char buffer[1024];
    int found_term = 0;
    
    while (!found_term) {
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
            found_term = 1;
        }
        
        strbuf_grow(sb, len);
        memcpy(sb->buf + sb->len, buffer, len);
        sb->len += len;
    }
    
    sb->buf[sb->len] = '\0';
    return 0;
}