void Variant::interpolate(const Variant &a, const Variant &b, float c, Variant &r_dst) {
    if (a.type != b.type) {
        if (a.is_num() && b.is_num()) {
            r_dst = (1.0 - c) * (real_t)a + (real_t)b * c;
        } else {
            r_dst = a;
        }
        return;
    }

    switch (a.type) {
        // ... (keep other cases the same until POOL_VECTOR2_ARRAY)

        case POOL_VECTOR2_ARRAY: {
            const PoolVector<Vector2> &arr_a = *reinterpret_cast<const PoolVector<Vector2>*>(a._data._mem);
            const PoolVector<Vector2> &arr_b = *reinterpret_cast<const PoolVector<Vector2>*>(b._data._mem);
            const int sz = arr_a.size();
            
            if (sz == 0 || arr_b.size() != sz) {
                r_dst = a;
                return;
            }

            PoolVector<Vector2> v;
            v.resize(sz);
            PoolVector<Vector2>::Write vw = v.write();
            PoolVector<Vector2>::Read ar = arr_a.read();
            PoolVector<Vector2>::Read br = arr_b.read();

            for (int i = 0; i < sz; i++) {
                vw[i] = ar[i].linear_interpolate(br[i], c);
            }
            r_dst = v;
        } return;

        case POOL_VECTOR3_ARRAY: {
            const PoolVector<Vector3> &arr_a = *reinterpret_cast<const PoolVector<Vector3>*>(a._data._mem);
            const PoolVector<Vector3> &arr_b = *reinterpret_cast<const PoolVector<Vector3>*>(b._data._mem);
            const int sz = arr_a.size();
            
            if (sz == 0 || arr_b.size() != sz) {
                r_dst = a;
                return;
            }

            PoolVector<Vector3> v;
            v.resize(sz);
            PoolVector<Vector3>::Write vw = v.write();
            PoolVector<Vector3>::Read ar = arr_a.read();
            PoolVector<Vector3>::Read br = arr_b.read();

            for (int i = 0; i < sz; i++) {
                vw[i] = ar[i].linear_interpolate(br[i], c);
            }
            r_dst = v;
        } return;

        // ... (keep remaining cases the same)
    }
}