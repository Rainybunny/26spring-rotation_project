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
        case NIL: r_dst = Variant(); return;
        case BOOL: r_dst = a; return;
        case INT: {
            int64_t va = a._data._int;
            int64_t vb = b._data._int;
            r_dst = (va != vb) ? int((1.0 - c) * va + vb * c) : a;
            return;
        }
        case REAL: {
            real_t va = a._data._real;
            real_t vb = b._data._real;
            r_dst = (1.0 - c) * va + vb * c;
            return;
        }
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
            dst.resize(csize + 1);
            dst[csize] = 0;
            
            const CharType *sa_ptr = sa.ptr();
            const CharType *sb_ptr = sb.ptr();
            CharType *dst_ptr = dst.ptrw();
            
            int split = csize / 2;
            int min_len = MIN(sa_len, sb_len);
            
            for (int i = 0; i < split; i++) {
                dst_ptr[i] = (i < sa_len) ? sa_ptr[i] : 
                            ((i < sb_len) ? sb_ptr[i] : ' ');
            }
            
            for (int i = split; i < csize; i++) {
                dst_ptr[i] = (i < sb_len) ? sb_ptr[i] : 
                            ((i < sa_len) ? sa_ptr[i] : ' ');
            }
            
            r_dst = dst;
            return;
        }
        case VECTOR2: {
            r_dst = reinterpret_cast<const Vector2 *>(a._data._mem)->linear_interpolate(
                *reinterpret_cast<const Vector2 *>(b._data._mem), c);
            return;
        }
        case RECT2: {
            r_dst = Rect2(
                reinterpret_cast<const Rect2 *>(a._data._mem)->position.linear_interpolate(
                    reinterpret_cast<const Rect2 *>(b._data._mem)->position, c),
                reinterpret_cast<const Rect2 *>(a._data._mem)->size.linear_interpolate(
                    reinterpret_cast<const Rect2 *>(b._data._mem)->size, c));
            return;
        }
        case VECTOR3: {
            r_dst = reinterpret_cast<const Vector3 *>(a._data._mem)->linear_interpolate(
                *reinterpret_cast<const Vector3 *>(b._data._mem), c);
            return;
        }
        case TRANSFORM2D: {
            r_dst = a._data._transform2d->interpolate_with(*b._data._transform2d, c);
            return;
        }
        case PLANE: r_dst = a; return;
        case QUAT: {
            r_dst = reinterpret_cast<const Quat *>(a._data._mem)->slerp(
                *reinterpret_cast<const Quat *>(b._data._mem), c);
            return;
        }
        case AABB: {
            r_dst = ::AABB(
                a._data._aabb->position.linear_interpolate(b._data._aabb->position, c),
                a._data._aabb->size.linear_interpolate(b._data._aabb->size, c));
            return;
        }
        case BASIS: {
            r_dst = Transform(*a._data._basis).interpolate_with(
                Transform(*b._data._basis), c).basis;
            return;
        }
        case TRANSFORM: {
            r_dst = a._data._transform->interpolate_with(*b._data._transform, c);
            return;
        }
        case COLOR: {
            r_dst = reinterpret_cast<const Color *>(a._data._mem)->linear_interpolate(
                *reinterpret_cast<const Color *>(b._data._mem), c);
            return;
        }
        case NODE_PATH: r_dst = a; return;
        case _RID: r_dst = a; return;
        case OBJECT: r_dst = a; return;
        case DICTIONARY: return;
        case ARRAY: r_dst = a; return;
        case POOL_BYTE_ARRAY: r_dst = a; return;
        case POOL_INT_ARRAY: r_dst = a; return;
        case POOL_REAL_ARRAY: r_dst = a; return;
        case POOL_STRING_ARRAY: r_dst = a; return;
        case POOL_VECTOR2_ARRAY: {
            const PoolVector<Vector2> *arr_a = reinterpret_cast<const PoolVector<Vector2> *>(a._data._mem);
            const PoolVector<Vector2> *arr_b = reinterpret_cast<const PoolVector<Vector2> *>(b._data._mem);
            int sz = arr_a->size();
            if (sz == 0 || arr_b->size() != sz) {
                r_dst = a;
            } else {
                PoolVector<Vector2> v;
                v.resize(sz);
                PoolVector<Vector2>::Write vw = v.write();
                PoolVector<Vector2>::Read ar = arr_a->read();
                PoolVector<Vector2>::Read br = arr_b->read();
                for (int i = 0; i < sz; i++) {
                    vw[i] = ar[i].linear_interpolate(br[i], c);
                }
                r_dst = v;
            }
            return;
        }
        case POOL_VECTOR3_ARRAY: {
            const PoolVector<Vector3> *arr_a = reinterpret_cast<const PoolVector<Vector3> *>(a._data._mem);
            const PoolVector<Vector3> *arr_b = reinterpret_cast<const PoolVector<Vector3> *>(b._data._mem);
            int sz = arr_a->size();
            if (sz == 0 || arr_b->size() != sz) {
                r_dst = a;
            } else {
                PoolVector<Vector3> v;
                v.resize(sz);
                PoolVector<Vector3>::Write vw = v.write();
                PoolVector<Vector3>::Read ar = arr_a->read();
                PoolVector<Vector3>::Read br = arr_b->read();
                for (int i = 0; i < sz; i++) {
                    vw[i] = ar[i].linear_interpolate(br[i], c);
                }
                r_dst = v;
            }
            return;
        }
        case POOL_COLOR_ARRAY: r_dst = a; return;
        default: r_dst = a;
    }
}