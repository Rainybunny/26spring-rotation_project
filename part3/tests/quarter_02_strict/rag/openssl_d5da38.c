int OBJ_obj2txt(char *buf, int buf_len, const ASN1_OBJECT *a, int no_name)
{
    int i, idx = 0, n = 0, len, nid;
    unsigned long l;
    unsigned char *p;
    const char *s;
    char tbuf[32];
    char *tptr;

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
        while (idx < a->length) {
            l |= (p[idx] & 0x7f);
            if (!(p[idx] & 0x80)) break;
            l <<= 7L;
            idx++;
        }
        idx++;
        i = (int)(l / 40);
        if (i > 2) i = 2;
        l -= (long)(i * 40);

        /* Optimized number formatting */
        tptr = tbuf;
        *tptr++ = '0' + i;
        *tptr++ = '.';
        if (l >= 10) {
            *tptr++ = '0' + (l / 10);
            l %= 10;
        }
        *tptr++ = '0' + l;
        *tptr = '\0';
        i = tptr - tbuf;

        if (buf_len > 0) {
            int copy_len = (i < buf_len) ? i : buf_len;
            memcpy(buf, tbuf, copy_len);
            buf_len -= copy_len;
            buf += copy_len;
        }
        n += i;

        l = 0;
        for (; idx < len; idx++) {
            l |= p[idx] & 0x7f;
            if (!(p[idx] & 0x80)) {
                /* Optimized number formatting */
                tptr = tbuf;
                *tptr++ = '.';
                if (l >= 100) {
                    *tptr++ = '0' + (l / 100);
                    l %= 100;
                }
                if (l >= 10) {
                    *tptr++ = '0' + (l / 10);
                    l %= 10;
                }
                *tptr++ = '0' + l;
                *tptr = '\0';
                i = tptr - tbuf;

                if (buf_len > 0) {
                    int copy_len = (i < buf_len) ? i : buf_len;
                    memcpy(buf, tbuf, copy_len);
                    buf_len -= copy_len;
                    buf += copy_len;
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
            int copy_len = (slen < buf_len) ? slen : buf_len;
            memcpy(buf, s, copy_len);
            n = slen;
        }
    }
    if (buf_len > 0) {
        buf[buf_len - 1] = '\0';
    }
    return n;
}