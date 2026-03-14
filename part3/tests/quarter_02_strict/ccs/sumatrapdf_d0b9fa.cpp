int StrVec::FindI(const char* s, int startAt) const {
    int sLen = str::Leni(s);
    auto end = this->end();
    for (auto it = this->begin() + startAt; it != end; it++) {
        char* s2 = *it;
        if (s2 && str::Leni(s2) == sLen && str::EqI(s, s2)) {
            return it.idx;
        }
    }
    return -1;
}