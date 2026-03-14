// https://pubs.opengroup.org/onlinepubs/9699919799/functions/memmove.html
void* memmove(void* dest, void const* src, size_t n)
{
    if (((FlatPtr)dest - (FlatPtr)src) >= n)
        return memcpy(dest, src, n);

    u8* pd = (u8*)dest;
    u8 const* ps = (u8 const*)src;
    
    // Copy backwards in word-sized chunks when possible
    if (n >= sizeof(uintptr_t)) {
        // Align destination to word boundary
        size_t align = (FlatPtr)pd % sizeof(uintptr_t);
        if (align) {
            align = sizeof(uintptr_t) - align;
            while (align-- && n--)
                *--pd = *--ps;
        }
        
        // Copy words
        uintptr_t* wpd = (uintptr_t*)pd;
        uintptr_t const* wps = (uintptr_t const*)ps;
        for (; n >= sizeof(uintptr_t); n -= sizeof(uintptr_t))
            *--wpd = *--wps;
            
        pd = (u8*)wpd;
        ps = (u8 const*)wps;
    }
    
    // Copy remaining bytes
    while (n--)
        *--pd = *--ps;
        
    return dest;
}