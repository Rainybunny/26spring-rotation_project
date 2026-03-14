int OBJ_obj2txt(char *buf, int buf_len, const ASN1_OBJECT *a, int no_name)
{
    int i, idx = 0, n = 0, len, nid;
    unsigned long l;
    const unsigned char *p;
    const char *s;
    char tbuf[32];
    int a_length;
    const unsigned char *a_data;

    if (buf_len <= 0) return 0;

    if ((a == NULL) || (a->data == NULL)) {
        buf[0] = '\0';
        return 0;
    }

    /* Cache frequently accessed fields */
    a_length = a->length;
    a_data = a->data;

    nid = OBJ_obj2nid(a);
    if ((nid == NID_undef) || no_name) {
        len = a_length;
        p = a_data;

        idx = 0;
        l = 0;
        while (idx < a_length) {
            l |= (p[idx] & 0x7f);
            if (!(p[idx] & 0x80)) break;
            l <<= 7L;
            idx++;
        }
        idx++;
        i = (int)(l / 40);
        if (i > 2) i = 2;
        l -= (long)(i * 40);

        /* Optimized sprintf/strlen sequence */
        i = snprintf(tbuf, sizeof(tbuf), "%d.%lu", i, l);
        if (buf_len > 0) {
            int copy_len = (i < buf_len) ? i : buf_len - 1;
            memcpy(buf, tbuf, copy_len);
            buf += copy_len;
            buf_len -= copy_len;
        }
        n += i;

        l = 0;
        for (; idx < len; idx++) {
            l |= p[idx] & 0x7f;
            if (!(p[idx] & 0x80)) {
                i = snprintf(tbuf, sizeof(tbuf), ".%lu", l);
                if (buf_len > 0) {
                    int copy_len = (i < buf_len) ? i : buf_len - 1;
                    memcpy(buf, tbuf, copy_len);
                    buf += copy_len;
                    buf_len -= copy_len;
                }
                n += i;
                l = 0;
            }
            l <<= 7L;
        }
    } else {
        s = OBJ_nid2ln(nid);
        if (s == NULL)
            s = OBJ_nid2sn(nid);
        if (s) {
            int slen = strlen(s);
            n = (slen < buf_len) ? slen : buf_len - 1;
            memcpy(buf, s, n);
            buf_len -= n;
        }
    }
    if (buf_len > 0)
        buf[0] = '\0';
    return n;
}