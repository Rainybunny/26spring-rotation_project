int
_zip_set_name(struct zip *za, zip_uint64_t idx, const char *name, zip_flags_t flags)
{
    struct zip_entry *e;
    struct zip_string *str;
    int changed;
    zip_int64_t i;
    size_t name_len = 0;

    if (idx >= za->nentry) {
        _zip_error_set(&za->error, ZIP_ER_INVAL, 0);
        return -1;
    }

    if (ZIP_IS_RDONLY(za)) {
        _zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
        return -1;
    }

    e = za->entry+idx;
    struct zip_dirent *orig = e->orig;
    struct zip_dirent *changes = e->changes;

    if (name && *name != '\0') {
        name_len = strlen(name);
        /* TODO: check for string too long */
        if ((str=_zip_string_new((const zip_uint8_t *)name, (zip_uint16_t)name_len, flags, &za->error)) == NULL)
            return -1;
        if ((flags & ZIP_FL_ENCODING_ALL) == ZIP_FL_ENC_GUESS && _zip_guess_encoding(str, ZIP_ENCODING_UNKNOWN) == ZIP_ENCODING_UTF8_GUESSED)
            str->encoding = ZIP_ENCODING_UTF8_KNOWN;
    }
    else
        str = NULL;

    /* TODO: encoding flags needed for CP437? */
    if ((i=_zip_name_locate(za, name, 0, NULL)) >= 0 && (zip_uint64_t)i != idx) {
        _zip_string_free(str);
        _zip_error_set(&za->error, ZIP_ER_EXISTS, 0);
        return -1;
    }

    /* no effective name change */
    if (i>=0 && (zip_uint64_t)i == idx) {
        _zip_string_free(str);
        return 0;
    }

    if (changes) {
        _zip_string_free(changes->filename);
        changes->filename = NULL;
        changes->changed &= ~ZIP_DIRENT_FILENAME;
    }

    changed = (orig ? !_zip_string_equal(orig->filename, str) : 1);
    
    if (changed) {
        if (changes == NULL) {
            if ((changes=_zip_dirent_clone(orig)) == NULL) {
                _zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
                _zip_string_free(str);
                return -1;
            }
            e->changes = changes;
        }
        changes->filename = str;
        changes->changed |= ZIP_DIRENT_FILENAME;
    }
    else {
        _zip_string_free(str);
        if (changes && changes->changed == 0) {
            _zip_dirent_free(changes);
            e->changes = changes = NULL;
        }
    }

    return 0;
}