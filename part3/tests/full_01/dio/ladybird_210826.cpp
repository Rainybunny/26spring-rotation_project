// https://pubs.opengroup.org/onlinepubs/9699919799/functions/memmove.html
void* memmove(void* dest, void const* src, size_t n)
{
    if (((FlatPtr)dest - (FlatPtr)src) >= n)
        return memcpy(dest, src, n);

    u8* pd = (u8*)dest;
    u8 const* ps = (u8 const*)src;
    
    // For large blocks, use word-sized copies when possible
    if (n >= sizeof(uintptr_t)) {
        // Check if we can do word-aligned copies
        if (((FlatPtr)pd & (sizeof(uintptr_t)-1)) == 0 && 
            ((FlatPtr)ps & (sizeof(uintptr_t)-1)) == 0) {
            uintptr_t* wpd = (uintptr_t*)(pd + n);
            uintptr_t const* wps = (uintptr_t const*)(ps + n);
            size_t word_count = n / sizeof(uintptr_t);
            n %= sizeof(uintptr_t);
            
            while (word_count--)
                *--wpd = *--wps;
            
            pd = (u8*)wpd;
            ps = (u8 const*)wps;
        }
    }
    
    // Handle remaining bytes
    for (pd += n, ps += n; n--;)
        *--pd = *--ps;
    
    return dest;
}