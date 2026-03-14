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
        // ... (keep all other cases the same until STRING case)
        
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
            int min_len = MIN(sa_len, sb_len);
            
            // Optimized copy for overlapping ranges
            if (split <= min_len) {
                dst.substr(0, split) = sa.substr(0, split);
                dst.substr(split) = sb.substr(split, csize - split);
            } else {
                // Handle non-overlapping parts
                if (sa_len > split) {
                    dst.substr(0, split) = sa.substr(0, split);
                    dst.substr(split, MIN(sa_len, csize) - split) = sa.substr(split, MIN(sa_len, csize) - split);
                }
                if (sb_len > split) {
                    int copy_len = MIN(sb_len, csize) - split;
                    if (copy_len > 0) {
                        dst.substr(MAX(split, sa_len), copy_len) = sb.substr(split, copy_len);
                    }
                }
            }
            
            r_dst = dst;
        } return;

        // ... (keep all remaining cases the same)
    }
}