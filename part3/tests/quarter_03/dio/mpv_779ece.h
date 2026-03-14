static inline bool bstr_equals(struct bstr str1, struct bstr str2)
{
    if (str1.start == str2.start)
        return str1.len == str2.len;
    return str1.len == str2.len && bstrcmp(str1, str2) == 0;
}