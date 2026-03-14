int OBJ_obj2txt(char *buf, int buf_len, const ASN1_OBJECT *a, int no_name)
{
    int i,idx=0,n=0,len,nid;
    unsigned long l;
    unsigned char *p;
    const char *s;
    char tbuf[32];
    char *tptr;

    if (buf_len <= 0) return(0);

    if ((a == NULL) || (a->data == NULL)) {
        buf[0]='\0';
        return(0);
    }

    nid=OBJ_obj2nid(a);
    if ((nid == NID_undef) || no_name) {
        len=a->length;
        p=a->data;

        idx=0;
        l=0;
        while (idx < a->length) {
            l|=(p[idx]&0x7f);
            if (!(p[idx] & 0x80)) break;
            l<<=7L;
            idx++;
        }
        idx++;
        i=(int)(l/40);
        if (i > 2) i=2;
        l-=(long)(i*40);

        /* Optimized first number formatting */
        tptr = tbuf;
        if (i >= 10) {
            *tptr++ = '0' + i/10;
            i %= 10;
        }
        *tptr++ = '0' + i;
        *tptr++ = '.';
        /* Optimized second number formatting */
        unsigned long tmp = l;
        char *start = tptr;
        do {
            *tptr++ = '0' + (tmp % 10);
            tmp /= 10;
        } while (tmp > 0);
        *tptr = '\0';
        /* Reverse the digits */
        char *end = tptr - 1;
        while (start < end) {
            char c = *start;
            *start++ = *end;
            *end-- = c;
        }
        i = tptr - tbuf;
        if (buf_len > 0) {
            strncpy(buf, tbuf, buf_len);
            if (i > buf_len) i = buf_len;
        }
        buf_len -= i;
        buf += i;
        n += i;

        l=0;
        for (; idx<len; idx++) {
            l|=p[idx]&0x7f;
            if (!(p[idx] & 0x80)) {
                /* Optimized subsequent number formatting */
                tptr = tbuf;
                *tptr++ = '.';
                unsigned long tmp = l;
                char *start = tptr;
                do {
                    *tptr++ = '0' + (tmp % 10);
                    tmp /= 10;
                } while (tmp > 0);
                *tptr = '\0';
                /* Reverse the digits */
                char *end = tptr - 1;
                while (start < end) {
                    char c = *start;
                    *start++ = *end;
                    *end-- = c;
                }
                i = tptr - tbuf;
                if (buf_len > 0) {
                    strncpy(buf, tbuf, buf_len);
                    if (i > buf_len) i = buf_len;
                }
                buf_len -= i;
                buf += i;
                n += i;
                l=0;
            }
            l<<=7L;
        }
    } else {
        s=OBJ_nid2ln(nid);
        if (s == NULL)
            s=OBJ_nid2sn(nid);
        if (buf_len > 0) {
            strncpy(buf, s, buf_len);
            n = strlen(s);
            if (n >= buf_len) n = buf_len - 1;
        } else {
            n = strlen(s);
        }
    }
    if (buf_len > 0) {
        buf[n]='\0';
    }
    return(n);
}