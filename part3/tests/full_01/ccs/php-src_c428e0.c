static void sdl_serialize_type(sdlTypePtr type, HashTable *tmp_encoders, HashTable *tmp_types, smart_str *out)
{
    int i;
    HashTable *tmp_elements = NULL;
    sdlRestrictions *restrictions = type->restrictions;
    HashTable *elements = type->elements;
    HashTable *attributes = type->attributes;

    /* Cache type properties */
    WSDL_CACHE_PUT_1(type->kind, out);
    sdl_serialize_string(type->name, out);
    sdl_serialize_string(type->namens, out);
    sdl_serialize_string(type->def, out);
    sdl_serialize_string(type->fixed, out);
    sdl_serialize_string(type->ref, out);
    WSDL_CACHE_PUT_1(type->nillable,    		out);
    WSDL_CACHE_PUT_1(type->form,        		out);
    sdl_serialize_encoder_ref(type->encode, tmp_encoders, out);

    /* Handle restrictions */
    if (restrictions) {
        WSDL_CACHE_PUT_1(1, out);
        sdl_serialize_resriction_int(restrictions->minExclusive, out);
        sdl_serialize_resriction_int(restrictions->minInclusive, out);
        sdl_serialize_resriction_int(restrictions->maxExclusive, out);
        sdl_serialize_resriction_int(restrictions->maxInclusive, out);
        sdl_serialize_resriction_int(restrictions->totalDigits,  out);
        sdl_serialize_resriction_int(restrictions->fractionDigits, out);
        sdl_serialize_resriction_int(restrictions->length,       out);
        sdl_serialize_resriction_int(restrictions->minLength,    out);
        sdl_serialize_resriction_int(restrictions->maxLength,    out);
        sdl_serialize_resriction_char(restrictions->whiteSpace,  out);
        sdl_serialize_resriction_char(restrictions->pattern,     out);

        i = restrictions->enumeration ? zend_hash_num_elements(restrictions->enumeration) : 0;
        WSDL_CACHE_PUT_INT(i, out);
        if (i > 0) {
            sdlRestrictionCharPtr *tmp;
            HashTable *enumeration = restrictions->enumeration;
            
            zend_hash_internal_pointer_reset(enumeration);
            while (zend_hash_get_current_data(enumeration, (void**)&tmp) == SUCCESS) {
                sdl_serialize_resriction_char(*tmp, out);
                sdl_serialize_key(enumeration, out);
                zend_hash_move_forward(enumeration);
            }
        }
    } else {
        WSDL_CACHE_PUT_1(0, out);
    }

    /* Handle elements */
    i = elements ? zend_hash_num_elements(elements) : 0;
    WSDL_CACHE_PUT_INT(i, out);
    if (i > 0) {
        sdlTypePtr *tmp;
        
        tmp_elements = emalloc(sizeof(HashTable));
        zend_hash_init(tmp_elements, 0, NULL, NULL, 0);

        zend_hash_internal_pointer_reset(elements);
        while (zend_hash_get_current_data(elements, (void**)&tmp) == SUCCESS) {
            sdl_serialize_key(elements, out);
            sdl_serialize_type(*tmp, tmp_encoders, tmp_types, out);
            zend_hash_add(tmp_elements, (char*)tmp, sizeof(*tmp), &i, sizeof(int), NULL);
            i--;
            zend_hash_move_forward(elements);
        }
    }

    /* Handle attributes */
    i = attributes ? zend_hash_num_elements(attributes) : 0;
    WSDL_CACHE_PUT_INT(i, out);
    if (i > 0) {
        sdlAttributePtr *tmp;
        
        zend_hash_internal_pointer_reset(attributes);
        while (zend_hash_get_current_data(attributes, (void**)&tmp) == SUCCESS) {
            sdl_serialize_key(attributes, out);
            sdl_serialize_attribute(*tmp, tmp_encoders, out);
            zend_hash_move_forward(attributes);
        }
    }

    /* Handle model */
    if (type->model) {
        WSDL_CACHE_PUT_1(1, out);
        sdl_serialize_model(type->model, tmp_types, tmp_elements, out);
    } else {
        WSDL_CACHE_PUT_1(0, out);
    }

    if (tmp_elements != NULL) {
        zend_hash_destroy(tmp_elements);
        efree(tmp_elements);
    }
}