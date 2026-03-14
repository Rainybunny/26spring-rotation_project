case STRING: {
    // Optimized string interpolation
    const String &sa = *reinterpret_cast<const String *>(a._data._mem);
    const String &sb = *reinterpret_cast<const String *>(b._data._mem);
    
    int sa_len = sa.length();
    int sb_len = sb.length();
    int dst_len = sb_len * c + sa_len * (1.0 - c);
    
    if (dst_len == 0) {
        r_dst = "";
        return;
    }
    
    String dst;
    dst.resize(dst_len + 1);
    dst[dst_len] = 0;
    
    int split = dst_len / 2;
    const CharType *sa_ptr = sa.ptr();
    const CharType *sb_ptr = sb.ptr();
    CharType *dst_ptr = dst.ptrw();
    
    // First half - prefer string A
    int i = 0;
    for (; i < split && i < dst_len; i++) {
        dst_ptr[i] = (i < sa_len) ? sa_ptr[i] : 
                    ((i < sb_len) ? sb_ptr[i] : ' ');
    }
    
    // Second half - prefer string B
    for (; i < dst_len; i++) {
        dst_ptr[i] = (i < sb_len) ? sb_ptr[i] : 
                    ((i < sa_len) ? sa_ptr[i] : ' ');
    }
    
    r_dst = dst;
}