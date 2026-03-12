int OBJ_obj2txt(char *buf, int buf_len, const ASN1_OBJECT *a, int no_name)
{
    int i,idx=0,n=0,len,nid;
    unsigned long l;
    unsigned char *p;
    const char *s;
    char *tbuf_ptr;
    char tbuf[32];

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

        // Fast custom number formatting
        tbuf_ptr = tbuf;
        if (i >= 10) {
            *tbuf_ptr++ = '0' + i/10;
            i %= 10;
        }
        *tbuf_ptr++ = '0' + i;
        *tbuf_ptr++ = '.';
        
        unsigned long tmp = l;
        int digits = 1;
        while (tmp >= 10) {
            tmp /= 10;
            digits++;
        }
        tmp = l;
        tbuf_ptr += digits;
        *tbuf_ptr = '\0';
        do {
            *--tbuf_ptr = '0' + (tmp % 10);
            tmp /= 10;
        } while (tmp != 0);
        tbuf_ptr += digits + 1; // point past null

        i = tbuf_ptr - tbuf - 1; // don't count null
        if (buf_len > 0) {
            int copy_len = i < buf_len ? i : buf_len;
            memcpy(buf, tbuf, copy_len);
        }
        buf_len -= i;
        buf += i;
        n += i;

        l=0;
        for (; idx<len; idx++) {
            l|=p[idx]&0x7f;
            if (!(p[idx] & 0x80)) {
                // Fast custom number formatting for components
                tbuf_ptr = tbuf;
                *tbuf_ptr++ = '.';
                
                unsigned long tmp = l;
                int digits = 1;
                while (tmp >= 10) {
                    tmp /= 10;
                    digits++;
                }
                tmp = l;
                tbuf_ptr += digits;
                *tbuf_ptr = '\0';
                do {
                    *--tbuf_ptr = '0' + (tmp % 10);
                    tmp /= 10;
                } while (tmp != 0);
                tbuf_ptr += digits + 1; // point past null

                i = tbuf_ptr - tbuf - 1;
                if (buf_len > 0) {
                    int copy_len = i < buf_len ? i : buf_len;
                    memcpy(buf, tbuf, copy_len);
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
        if (s) {
            int slen = strlen(s);
            int copy_len = slen < buf_len ? slen : buf_len;
            memcpy(buf, s, copy_len);
            n = slen;
        }
    }
    if (buf_len > 0) {
        buf[buf_len-1]='\0';
    } else if (n > 0) {
        buf[0]='\0';
    }
    return(n);
}