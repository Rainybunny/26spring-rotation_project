case STRING: {
    const String &sa = *reinterpret_cast<const String *>(a._data._mem);
    const String &sb = *reinterpret_cast<const String *>(b._data._mem);
    
    int sa_len = sa.length();
    int sb_len = sb.length();
    int csize = sb_len * c + sa_len * (1.0 - c);
    
    if (csize == 0) {
        r_dst = "";
        return;
    }
    
    String dst;
    dst.resize(csize);
    
    const CharType *sa_ptr = sa.ptr();
    const CharType *sb_ptr = sb.ptr();
    CharType *dst_ptr = dst.ptrw();
    
    int split = csize / 2;
    int min_len = MIN(sa_len, sb_len);
    
    for (int i = 0; i < csize; i++) {
        CharType chr = ' ';
        
        if (i < min_len) {
            chr = (i < split) ? sa_ptr[i] : sb_ptr[i];
        } else if (i < sa_len) {
            chr = sa_ptr[i];
        } else if (i < sb_len) {
            chr = sb_ptr[i];
        }
        
        dst_ptr[i] = chr;
    }
    
    r_dst = dst;
}
return;