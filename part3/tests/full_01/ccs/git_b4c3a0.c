int xdl_recmatch(const char *l1, long s1, const char *l2, long s2, long flags) {
    int i1 = 0, i2 = 0;
    static const unsigned char is_whitespace[256] = {
        [' '] = 1, ['\t'] = 1, ['\n'] = 1, ['\r'] = 1, ['\v'] = 1, ['\f'] = 1
    };

    if (!(flags & XDF_WHITESPACE_FLAGS))
        return s1 == s2 && !memcmp(l1, l2, s1);

    if (flags & XDF_IGNORE_WHITESPACE) {
        goto skip_ws;
        while (i1 < s1 && i2 < s2) {
            if (l1[i1++] != l2[i2++])
                return 0;
        skip_ws:
            while (i1 < s1 && is_whitespace[(unsigned char)l1[i1]])
                i1++;
            while (i2 < s2 && is_whitespace[(unsigned char)l2[i2]])
                i2++;
        }
    } else if (flags & XDF_IGNORE_WHITESPACE_CHANGE) {
        while (i1 < s1 && i2 < s2) {
            if (is_whitespace[(unsigned char)l1[i1]] && is_whitespace[(unsigned char)l2[i2]]) {
                while (i1 < s1 && is_whitespace[(unsigned char)l1[i1]])
                    i1++;
                while (i2 < s2 && is_whitespace[(unsigned char)l2[i2]])
                    i2++;
                continue;
            }
            if (l1[i1++] != l2[i2++])
                return 0;
        }
    } else if (flags & XDF_IGNORE_WHITESPACE_AT_EOL) {
        while (i1 < s1 && i2 < s2 && l1[i1++] == l2[i2++])
            ; /* keep going */
    }

    /* Check remaining characters */
    while (i1 < s1 && is_whitespace[(unsigned char)l1[i1]])
        i1++;
    if (i1 < s1)
        return 0;
    while (i2 < s2 && is_whitespace[(unsigned char)l2[i2]])
        i2++;
    return (i2 == s2);
}