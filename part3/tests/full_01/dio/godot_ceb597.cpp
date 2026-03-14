void Variant::interpolate(const Variant &a, const Variant &b, float c, Variant &r_dst) {
    if (a.type != b.type) {
        if (a.is_num() && b.is_num()) {
            real_t va = a;
            real_t vb = b;
            r_dst = (1.0 - c) * va + vb * c;
        } else {
            r_dst = a;
        }
        return;
    }

    switch (a.type) {
        // ... other cases remain the same ...

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
            
            int split = csize / 2;
            const CharType *sa_ptr = sa.ptr();
            const CharType *sb_ptr = sb.ptr();
            CharType *dst_ptr = dst.ptrw();

            for (int i = 0; i < csize; i++) {
                CharType chr = ' ';
                if (i < split) {
                    chr = (i < sa_len) ? sa_ptr[i] : 
                          (i < sb_len) ? sb_ptr[i] : ' ';
                } else {
                    chr = (i < sb_len) ? sb_ptr[i] : 
                          (i < sa_len) ? sa_ptr[i] : ' ';
                }
                dst_ptr[i] = chr;
            }

            r_dst = dst;
        } return;

        // ... rest of the cases remain the same ...
    }
}