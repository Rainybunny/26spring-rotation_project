int OBJ_obj2txt(char *buf, int buf_len, const ASN1_OBJECT *a, int no_name)
{
    int i, idx = 0, n = 0, len, nid;
    unsigned long l;
    unsigned char *p;
    const char *s;
    char *end = buf + buf_len - 1;  /* Leave space for null terminator */
    char *ptr = buf;

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

        /* Format first two components directly */
        if (ptr < end) {
            char *tmp = ptr;
            if (i != 0) {
                *ptr++ = '0' + i;
                n++;
            }
            *ptr++ = '.';
            n++;
            if (l >= 10) {
                *ptr++ = '0' + (l / 10);
                n++;
            }
            *ptr++ = '0' + (l % 10);
            n++;
            if (ptr >= end) {
                ptr = tmp;
                n = 0;
            }
        }

        l = 0;
        for (; idx < len; idx++) {
            l |= p[idx] & 0x7f;
            if (!(p[idx] & 0x80)) {
                if (ptr < end) {
                    char *tmp = ptr;
                    *ptr++ = '.';
                    n++;
                    if (l >= 100) {
                        if (ptr < end) {
                            *ptr++ = '0' + (l / 100);
                            n++;
                        }
                        l %= 100;
                    }
                    if (l >= 10) {
                        if (ptr < end) {
                            *ptr++ = '0' + (l / 10);
                            n++;
                        }
                    }
                    if (ptr < end) {
                        *ptr++ = '0' + (l % 10);
                        n++;
                    }
                    if (ptr >= end) {
                        ptr = tmp;
                        n -= (ptr - tmp);
                    }
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
            while (*s && ptr < end) {
                *ptr++ = *s++;
                n++;
            }
        }
    }
    *ptr = '\0';
    return n;
}