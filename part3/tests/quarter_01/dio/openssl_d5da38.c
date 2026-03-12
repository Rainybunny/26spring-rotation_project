int OBJ_obj2txt(char *buf, int buf_len, const ASN1_OBJECT *a, int no_name)
{
    int i, idx = 0, n = 0, len, nid;
    unsigned long l;
    unsigned char *p;
    const char *s;
    char *end = buf + buf_len;

    if (buf_len <= 0) return 0;

    if ((a == NULL) || (a->data == NULL)) {
        buf[0] = '\0';
        return 0;
    }

    nid = OBJ_obj2nid(a);
    if ((nid == NID_undef) || no_name) {
        len = a->length;
        p = a->data;

        idx = 0;
        l = 0;
        while (idx < len) {
            l |= (p[idx] & 0x7f);
            if (!(p[idx] & 0x80)) break;
            l <<= 7L;
            idx++;
        }
        idx++;
        i = (int)(l / 40);
        if (i > 2) i = 2;
        l -= (long)(i * 40);

        /* Format first two components directly */
        n = snprintf(buf, buf_len, "%d.%lu", i, l);
        if (n < 0 || n >= buf_len) {
            buf[buf_len-1] = '\0';
            return buf_len-1;
        }
        buf += n;
        buf_len -= n;

        l = 0;
        for (; idx < len; idx++) {
            l |= p[idx] & 0x7f;
            if (!(p[idx] & 0x80)) {
                n = snprintf(buf, buf_len, ".%lu", l);
                if (n < 0 || n >= buf_len) {
                    buf[buf_len-1] = '\0';
                    return buf_len-1;
                }
                buf += n;
                buf_len -= n;
                l = 0;
            }
            l <<= 7L;
        }
    } else {
        s = OBJ_nid2ln(nid);
        if (s == NULL)
            s = OBJ_nid2sn(nid);
        n = strlen(s);
        if (buf_len > n) {
            strncpy(buf, s, buf_len);
        } else {
            strncpy(buf, s, buf_len-1);
            buf[buf_len-1] = '\0';
            n = buf_len-1;
        }
    }
    return n;
}