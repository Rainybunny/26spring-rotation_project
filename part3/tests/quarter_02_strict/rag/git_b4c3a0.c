int xdl_recmatch(const char *l1, long s1, const char *l2, long s2, long flags) {
    const char *p1 = l1, *p2 = l2;
    const char *end1 = l1 + s1, *end2 = l2 + s2;

    /* Fast path: no whitespace flags */
    if (!(flags & XDF_WHITESPACE_FLAGS))
        return s1 == s2 && !memcmp(l1, l2, s1);

    /* Handle different whitespace modes */
    if (flags & XDF_IGNORE_WHITESPACE) {
        while (1) {
            /* Skip whitespace in both strings */
            while (p1 < end1 && isspace(*p1)) p1++;
            while (p2 < end2 && isspace(*p2)) p2++;
            
            /* If we reached end in both, match */
            if (p1 >= end1 && p2 >= end2) return 1;
            /* If only one reached end, check remaining whitespace */
            if (p1 >= end1 || p2 >= end2) break;
            
            /* Compare next non-whitespace chars */
            if (*p1++ != *p2++) return 0;
        }
    } else if (flags & XDF_IGNORE_WHITESPACE_CHANGE) {
        while (p1 < end1 && p2 < end2) {
            if (isspace(*p1) && isspace(*p2)) {
                /* Skip all whitespace in both */
                while (p1 < end1 && isspace(*p1)) p1++;
                while (p2 < end2 && isspace(*p2)) p2++;
                continue;
            }
            if (*p1++ != *p2++) return 0;
        }
    } else if (flags & XDF_IGNORE_WHITESPACE_AT_EOL) {
        /* Compare until first mismatch */
        while (p1 < end1 && p2 < end2 && *p1 == *p2) {
            p1++; p2++;
        }
    }

    /* Check remaining characters (must be all whitespace) */
    while (p1 < end1 && isspace(*p1)) p1++;
    while (p2 < end2 && isspace(*p2)) p2++;
    return (p1 == end1 && p2 == end2);
}