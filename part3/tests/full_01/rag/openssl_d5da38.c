int OBJ_obj2txt(char *buf, int buf_len, const ASN1_OBJECT *a, int no_name)
{
    int i, idx = 0, n = 0, len, nid;
    unsigned long l;
    unsigned char *p;
    const char *s;
    char *bufp = buf;

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

        /* Direct write for first part */
        if (buf_len > 0) {
            if (i >= 10) {
                if (buf_len > 0) *bufp++ = '0' + i/10;
                if (buf_len > 1) *bufp++ = '0' + i%10;
                n += 2;
                buf_len -= 2;
            } else {
                if (buf_len > 0) *bufp++ = '0' + i;
                n++;
                buf_len--;
            }
            if (buf_len > 0) *bufp++ = '.';
            n++;
            buf_len--;
        }

        /* Direct write for second part */
        if (l != 0) {
            char numbuf[32];
            char *nump = numbuf;
            unsigned long tmp = l;
            while (tmp) {
                *nump++ = '0' + tmp % 10;
                tmp /= 10;
            }
            if (nump == numbuf) {
                *nump++ = '0';
            }
            while (nump > numbuf && buf_len > 0) {
                *bufp++ = *--nump;
                n++;
                buf_len--;
            }
        }

        l = 0;
        for (; idx < len; idx++) {
            l |= p[idx] & 0x7f;
            if (!(p[idx] & 0x80)) {
                if (buf_len > 0) *bufp++ = '.';
                n++;
                buf_len--;
                if (l != 0) {
                    char numbuf[32];
                    char *nump = numbuf;
                    unsigned long tmp = l;
                    while (tmp) {
                        *nump++ = '0' + tmp % 10;
                        tmp /= 10;
                    }
                    if (nump == numbuf) {
                        *nump++ = '0';
                    }
                    while (nump > numbuf && buf_len > 0) {
                        *bufp++ = *--nump;
                        n++;
                        buf_len--;
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
            while (*s && buf_len > 1) {
                *bufp++ = *s++;
                n++;
                buf_len--;
            }
        }
    }
    if (buf_len > 0) *bufp = '\0';
    return n;
}