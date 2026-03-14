void Variant::interpolate(const Variant &a, const Variant &b, float c, Variant &r_dst) {
    if (a.type != b.type) {
        if (a.is_num() && b.is_num()) {
            // Optimized numeric interpolation for different types
            r_dst = a._data._real * (1.0f - c) + b._data._real * c;
            return;
        }
        r_dst = a;
        return;
    }

    switch (a.type) {
        case INT: {
            int64_t va = a._data._int;
            int64_t vb = b._data._int;
            if (va != vb) {
                // Optimized integer interpolation
                r_dst = static_cast<int64_t>(va + (vb - va) * c);
            } else {
                r_dst = a;
            }
            return;
        }
        case REAL: {
            // Optimized float interpolation
            r_dst = a._data._real + (b._data._real - a._data._real) * c;
            return;
        }
        // ... (rest of the cases remain the same)
        default: {
            r_dst = a;
        }
    }
}