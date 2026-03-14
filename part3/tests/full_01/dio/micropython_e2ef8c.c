mp_obj_t mp_obj_str_binary_op(mp_uint_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
    // check for modulo
    if (op == MP_BINARY_OP_MODULO) {
        mp_obj_t *args;
        mp_uint_t n_args;
        mp_obj_t dict = MP_OBJ_NULL;
        if (MP_OBJ_IS_TYPE(rhs_in, &mp_type_tuple)) {
            mp_obj_tuple_get(rhs_in, &n_args, &args);
        } else if (MP_OBJ_IS_TYPE(rhs_in, &mp_type_dict)) {
            args = NULL;
            n_args = 0;
            dict = rhs_in;
        } else {
            args = &rhs_in;
            n_args = 1;
        }
        return str_modulo_format(lhs_in, n_args, args, dict);
    }

    // from now on we need lhs type and data, so extract them
    mp_obj_type_t *lhs_type = mp_obj_get_type(lhs_in);
    GET_STR_DATA_LEN(lhs_in, lhs_data, lhs_len);

    // check for multiply
    if (op == MP_BINARY_OP_MULTIPLY) {
        mp_int_t n;
        if (!mp_obj_get_int_maybe(rhs_in, &n)) {
            return MP_OBJ_NULL;
        }
        if (n <= 0) {
            if (lhs_type == &mp_type_str) {
                return MP_OBJ_NEW_QSTR(MP_QSTR_);
            } else {
                return mp_const_empty_bytes;
            }
        }
        vstr_t vstr;
        vstr_init_len(&vstr, lhs_len * n);
        mp_seq_multiply(lhs_data, sizeof(*lhs_data), lhs_len, n, vstr.buf);
        return mp_obj_new_str_from_vstr(lhs_type, &vstr);
    }

    // Handle other operations
    const byte *rhs_data;
    mp_uint_t rhs_len;
    if (lhs_type == mp_obj_get_type(rhs_in)) {
        GET_STR_DATA_LEN(rhs_in, rhs_data_, rhs_len_);
        rhs_data = rhs_data_;
        rhs_len = rhs_len_;
    } else if (lhs_type == &mp_type_bytes) {
        mp_buffer_info_t bufinfo;
        if (!mp_get_buffer(rhs_in, &bufinfo, MP_BUFFER_READ)) {
            return MP_OBJ_NULL;
        }
        rhs_data = bufinfo.buf;
        rhs_len = bufinfo.len;
    } else {
        return MP_OBJ_NULL;
    }

    switch (op) {
        case MP_BINARY_OP_ADD:
        case MP_BINARY_OP_INPLACE_ADD: {
            // Optimized concatenation path
            mp_obj_str_t *o = m_new_obj(mp_obj_str_t);
            o->base.type = lhs_type;
            o->len = lhs_len + rhs_len;
            byte *buf = m_new(byte, o->len + 1);
            o->data = buf;
            memcpy(buf, lhs_data, lhs_len);
            memcpy(buf + lhs_len, rhs_data, rhs_len);
            buf[o->len] = '\0';
            o->hash = qstr_compute_hash(buf, o->len);
            return MP_OBJ_FROM_PTR(o);
        }

        case MP_BINARY_OP_IN:
            return mp_obj_new_bool(find_subbytes(lhs_data, lhs_len, rhs_data, rhs_len, 1) != NULL);

        case MP_BINARY_OP_EQUAL:
        case MP_BINARY_OP_LESS:
        case MP_BINARY_OP_LESS_EQUAL:
        case MP_BINARY_OP_MORE:
        case MP_BINARY_OP_MORE_EQUAL:
            return mp_obj_new_bool(mp_seq_cmp_bytes(op, lhs_data, lhs_len, rhs_data, rhs_len));
    }

    return MP_OBJ_NULL;
}