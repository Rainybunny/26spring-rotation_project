int strbuf_getwholeline(struct strbuf *sb, FILE *fp, int term)
{
    if (feof(fp))
        return EOF;

    strbuf_reset(sb);
    
    char buffer[1024];
    char *result;
    int found = 0;
    
    while ((result = fgets(buffer, sizeof(buffer), fp)) != NULL) {
        size_t len = strlen(buffer);
        char *term_pos = memchr(buffer, term, len);
        
        if (term_pos) {
            len = term_pos - buffer + 1;
            found = 1;
        }
        
        strbuf_grow(sb, len);
        memcpy(sb->buf + sb->len, buffer, len);
        sb->len += len;
        sb->buf[sb->len] = '\0';
        
        if (found)
            break;
    }
    
    if (sb->len == 0 && (result == NULL || feof(fp)))
        return EOF;
        
    return 0;
}