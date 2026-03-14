int StrVec::FindI(const char* s, int startAt) const {
    if (startAt < 0) {
        startAt = 0;
    }
    int sLen = str::Leni(s);
    auto [page, idxInPage] = PageForIdx(this, startAt);
    int remaining = size - startAt;
    
    while (remaining > 0 && page) {
        int pageRemaining = page->nStrings - idxInPage;
        int n = std::min(remaining, pageRemaining);
        
        for (int i = 0; i < n; i++) {
            int currLen;
            char* s2 = page->AtHelper(idxInPage + i, currLen);
            if (currLen == sLen && str::EqI(s, s2)) {
                return startAt + i;
            }
        }
        
        startAt += n;
        remaining -= n;
        page = page->next;
        idxInPage = 0;
    }
    return -1;
}