String String::from_utf8_with_replacement_character(StringView view)
{
    StringBuilder builder;
    builder.ensure_capacity(view.length());

    Utf8View utf8_view { view };
    auto it = utf8_view.begin();
    auto end = utf8_view.end();

    while (it != end) {
        if (is_ascii(*it)) {
            // Fast path for ASCII runs
            auto ascii_start = it.underlying_code_point_bytes().data();
            auto ascii_end = ascii_start;
            while (it != end && is_ascii(*it)) {
                ++ascii_end;
                ++it;
            }
            builder.append(StringView { ascii_start, static_cast<size_t>(ascii_end - ascii_start) });
        } else {
            // Slow path for non-ASCII
            builder.append_code_point(*it);
            ++it;
        }
    }

    return builder.to_string_without_validation();
}