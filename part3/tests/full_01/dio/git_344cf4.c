int strbuf_getwholeline(struct strbuf *sb, FILE *fp, int term)
{
    if (feof(fp))
        return EOF;

    strbuf_reset(sb);
    strbuf_grow(sb, 128);  // Pre-allocate initial buffer

    while (1) {
        int ch = fgetc(fp);
        if (ch == EOF) {
            if (sb->len == 0)
                return EOF;
            break;
        }
        
        if (sb->len + 1 >= sb->alloc)
            strbuf_grow(sb, 128);
            
        sb->buf[sb->len++] = ch;
        if (ch == term)
            break;
    }

    sb->buf[sb->len] = '\0';
    return 0;
}