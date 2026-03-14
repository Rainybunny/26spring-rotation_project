void PrimitiveString::resolve_rope_if_needed(EncodingPreference preference) const
{
    if (!m_is_rope)
        return;

    Vector<PrimitiveString const*> pieces;
    Vector<PrimitiveString const*> stack;
    stack.append(m_rhs);
    stack.append(m_lhs);
    while (!stack.is_empty()) {
        auto const* current = stack.take_last();
        if (current->m_is_rope) {
            stack.append(current->m_rhs);
            stack.append(current->m_lhs);
            continue;
        }
        pieces.append(current);
    }

    if (preference == EncodingPreference::UTF16) {
        Utf16Data code_units;
        for (auto const* current : pieces)
            code_units.extend(current->utf16_string().string());

        m_utf16_string = Utf16String::create(move(code_units));
        m_is_rope = false;
        m_lhs = nullptr;
        m_rhs = nullptr;
        return;
    }

    // Fast path: check if we need surrogate handling at all
    bool needs_surrogate_handling = false;
    {
        PrimitiveString const* prev = nullptr;
        for (auto const* current : pieces) {
            if (!prev) {
                prev = current;
                continue;
            }
            
            auto prev_view = prev->utf8_string_view();
            auto curr_view = current->utf8_string_view();
            
            if (prev_view.length() >= 3 && curr_view.length() >= 3 &&
                (static_cast<u8>(prev_view[prev_view.length() - 3]) & 0xf0) == 0xe0 &&
                (static_cast<u8>(curr_view[0]) & 0xf0) == 0xe0) {
                needs_surrogate_handling = true;
                break;
            }
            prev = current;
        }
    }

    StringBuilder builder;
    if (!needs_surrogate_handling) {
        // Fast path: simple concatenation
        for (auto const* current : pieces)
            builder.append(current->utf8_string());
    } else {
        // Slow path: handle surrogate pairs
        PrimitiveString const* previous = nullptr;
        for (auto const* current : pieces) {
            if (!previous) {
                builder.append(current->utf8_string());
                previous = current;
                continue;
            }

            auto current_string_as_utf8 = current->utf8_string_view();
            auto previous_string_as_utf8 = previous->utf8_string_view();

            if ((previous_string_as_utf8.length() < 3) || (current_string_as_utf8.length() < 3)) {
                builder.append(current_string_as_utf8);
                previous = current;
                continue;
            }

            if ((static_cast<u8>(previous_string_as_utf8[previous_string_as_utf8.length() - 3]) & 0xf0) != 0xe0) {
                builder.append(current_string_as_utf8);
                previous = current;
                continue;
            }

            if ((static_cast<u8>(current_string_as_utf8[0]) & 0xf0) != 0xe0) {
                builder.append(current_string_as_utf8);
                previous = current;
                continue;
            }

            auto high_surrogate = *Utf8View(previous_string_as_utf8.substring_view(previous_string_as_utf8.length() - 3)).begin();
            auto low_surrogate = *Utf8View(current_string_as_utf8).begin();

            if (!Utf16View::is_high_surrogate(high_surrogate) || !Utf16View::is_low_surrogate(low_surrogate)) {
                builder.append(current_string_as_utf8);
                previous = current;
                continue;
            }

            builder.trim(3);
            builder.append_code_point(Utf16View::decode_surrogate_pair(high_surrogate, low_surrogate));
            builder.append(current_string_as_utf8.substring_view(3));
            previous = current;
        }
    }

    m_utf8_string = builder.to_string_without_validation();
    m_is_rope = false;
    m_lhs = nullptr;
    m_rhs = nullptr;
}