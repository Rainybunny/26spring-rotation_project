int OBJ_obj2txt(char *buf, int buf_len, const ASN1_OBJECT *a, int no_name)
{
    int i, idx = 0, n = 0, len, nid;
    unsigned long l;
    unsigned char *p;
    const char *s;
    char tbuf[32];
    char *t;

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

        /* Optimized sprintf("%d.%lu") */
        t = tbuf;
        if (i >= 10) {
            *t++ = '0' + (i / 10);
            i %= 10;
        }
        *t++ = '0' + i;
        *t++ = '.';
        if (l >= 10) {
            *t++ = '0' + (l / 10);
            l %= 10;
        }
        *t++ = '0' + l;
        *t = '\0';
        i = t - tbuf;

        if (i > buf_len) i = buf_len;
        if (i > 0) {
            memcpy(buf, tbuf, i);
            buf_len -= i;
            buf += i;
            n += i;
        }

        l = 0;
        for (; idx < len; idx++) {
            l |= p[idx] & 0x7f;
            if (!(p[idx] & 0x80)) {
                /* Optimized sprintf(".%lu") */
                t = tbuf;
                *t++ = '.';
                if (l >= 10) {
                    *t++ = '0' + (l / 10);
                    l %= 10;
                }
                *t++ = '0' + l;
                *t = '\0';
                i = t - tbuf;

                if (i > buf_len) i = buf_len;
                if (i > 0) {
                    memcpy(buf, tbuf, i);
                    buf_len -= i;
                    buf += i;
                    n += i;
                }
                l = 0;
            }
            l <<= 7L;
        }
    } else {
        s = OBJ_nid2ln(nid);
        if (s == NULL)
            s = OBJ_nid2sn(nid);
        if (s) {
            i = strlen(s);
            if (i > buf_len) i = buf_len;
            if (i > 0) {
                memcpy(buf, s, i);
                n += i;
            }
        }
    }
    if (buf_len > 0) buf[0] = '\0';
    return n;
}