// https://pubs.opengroup.org/onlinepubs/9699919799/functions/memmove.html
void* memmove(void* dest, void const* src, size_t n)
{
    if (((FlatPtr)dest - (FlatPtr)src) >= n)
        return memcpy(dest, src, n);

    u8* pd = (u8*)dest;
    u8 const* ps = (u8 const*)src;
    
    // For small copies, just do byte-by-byte
    if (n < sizeof(u64)) {
        for (pd += n, ps += n; n--;)
            *--pd = *--ps;
        return dest;
    }

    // Copy trailing bytes if not aligned
    size_t align = (FlatPtr)pd % sizeof(u64);
    if (align) {
        size_t bytes = sizeof(u64) - align;
        if (bytes > n)
            bytes = n;
        n -= bytes;
        pd += bytes;
        ps += bytes;
        while (bytes--)
            *--pd = *--ps;
    }

    // Copy words
    u64* pdw = (u64*)pd;
    u64 const* psw = (u64 const*)ps;
    for (; n >= sizeof(u64); n -= sizeof(u64))
        *--pdw = *--psw;

    // Copy remaining bytes
    pd = (u8*)pdw;
    ps = (u8 const*)psw;
    while (n--)
        *--pd = *--ps;

    return dest;
}