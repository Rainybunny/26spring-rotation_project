void Variant::interpolate(const Variant &a, const Variant &b, float c, Variant &r_dst) {
    // ... [previous cases unchanged until STRING case] ...

		case STRING: {
			// Optimized string interpolation
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
			int copy_len;
			
			// Copy first part from sa
			copy_len = MIN(sa_len, split);
			if (copy_len > 0) {
				memcpy(dst.ptrw(), sa.ptr(), copy_len * sizeof(CharType));
			}
			
			// Copy remaining from sb
			copy_len = MIN(sb_len, csize) - split;
			if (copy_len > 0) {
				memcpy(dst.ptrw() + split, sb.ptr() + split, copy_len * sizeof(CharType));
			}
			
			// Fill any remaining space from sa if needed
			if (split + copy_len < csize) {
				int remaining = csize - (split + copy_len);
				if (remaining > 0 && sa_len > split + copy_len) {
					memcpy(dst.ptrw() + split + copy_len, 
						   sa.ptr() + split + copy_len, 
						   remaining * sizeof(CharType));
				}
			}
			
			r_dst = dst;
		}
			return;

    // ... [rest of cases unchanged] ...
}