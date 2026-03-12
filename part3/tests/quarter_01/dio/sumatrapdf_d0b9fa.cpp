int StrVec::FindI(const char* s, int startAt) const {
    if (startAt >= size) {
        return -1;
    }
    int sLen = str::Leni(s);
    auto [page, idxInPage] = PageForIdx(this, startAt);
    int remaining = size - startAt;
    
    while (remaining > 0 && page) {
        int pageStrings = page->nStrings - idxInPage;
        if (pageStrings > remaining) {
            pageStrings = remaining;
        }
        
        for (int i = 0; i < pageStrings; i++) {
            int currLen;
            char* s2 = page->AtHelper(idxInPage + i, currLen);
            if (currLen == sLen && str::EqI(s, s2)) {
                return startAt + i;
            }
        }
        
        startAt += pageStrings;
        remaining -= pageStrings;
        page = page->next;
        idxInPage = 0;
    }
    return -1;
}