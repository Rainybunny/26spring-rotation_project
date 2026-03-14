static inline bool bstr_equals(struct bstr str1, struct bstr str2)
{
    return str1.len == str2.len && 
           (str1.start == str2.start || 
            memcmp(str1.start, str2.start, str1.len) == 0);
}