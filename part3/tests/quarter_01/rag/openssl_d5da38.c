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

        /* Optimized sprintf replacement */
        tptr = tbuf;
        *tptr++ = '0' + i;
        *tptr++ = '.';
        tptr += sprintf(tptr, "%lu", l);
        i = tptr - tbuf;
        
        if (i > buf_len) i = buf_len;
        memcpy(buf, tbuf, i);
        buf_len -= i;
        buf += i;
        n += i;

        l=0;
        for (; idx<len; idx++) {
            l|=p[idx]&0x7f;
            if (!(p[idx] & 0x80)) {
                /* Optimized sprintf replacement */
                tptr = tbuf;
                *tptr++ = '.';
                tptr += sprintf(tptr, "%lu", l);
                i = tptr - tbuf;
                
                if (i > buf_len) i = buf_len;
                if (buf_len > 0)
                    memcpy(buf, tbuf, i);
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
            i = strlen(s);
            if (i >= buf_len) i = buf_len - 1;
            memcpy(buf, s, i);
            buf[i] = '\0';
            n = i;
        } else {
            n = strlen(s);
        }
    }
    if (buf_len > 0)
        buf[buf_len-1]='\0';
    return(n);
}