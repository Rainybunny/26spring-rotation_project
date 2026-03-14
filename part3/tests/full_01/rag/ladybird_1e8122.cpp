String String::from_utf8_with_replacement_character(StringView view)
{
    // Fast path: if input is valid UTF-8, just copy it directly
    if (Utf8View { view }.validate()) {
        String result;
        MUST(result.replace_with_new_string(view.length(), [&](Bytes buffer) {
            view.bytes().copy_to(buffer);
            return ErrorOr<void> {};
        }));
        return result;
    }

    // Slow path: process character by character with replacement
    StringBuilder builder;
    for (auto c : Utf8View { view })
        builder.append_code_point(c);
    return builder.to_string_without_validation();
}