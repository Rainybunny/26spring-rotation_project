// https://www.w3.org/TR/css-syntax-3/#convert-string-to-number
double Tokenizer::convert_a_string_to_a_number(StringView string)
{
    const char* ptr = string.characters_without_null_termination();
    const size_t length = string.length();
    size_t position = 0;

    // 1. Handle sign
    int sign = 1;
    if (position < length) {
        const u32 first_char = ptr[position];
        if (is_plus_sign(first_char) || is_hyphen_minus(first_char)) {
            sign = is_hyphen_minus(first_char) ? -1 : 1;
            position++;
        }
    }

    // 2. Integer part
    double integer_part = 0;
    while (position < length && is_ascii_digit(ptr[position])) {
        integer_part = integer_part * 10 + (ptr[position] - '0');
        position++;
    }

    // 3-4. Fractional part
    double fractional_part = 0;
    double fractional_scale = 1.0;
    if (position < length && is_full_stop(ptr[position])) {
        position++;
        while (position < length && is_ascii_digit(ptr[position])) {
            fractional_part = fractional_part * 10 + (ptr[position] - '0');
            fractional_scale *= 10;
            position++;
        }
    }

    // 5-7. Exponent
    int exponent_sign = 1;
    double exponent = 0;
    if (position < length && (is_e(ptr[position]) || is_E(ptr[position]))) {
        position++;
        if (position < length) {
            const u32 exp_sign_char = ptr[position];
            if (is_plus_sign(exp_sign_char) || is_hyphen_minus(exp_sign_char)) {
                exponent_sign = is_hyphen_minus(exp_sign_char) ? -1 : 1;
                position++;
            }
        }
        while (position < length && is_ascii_digit(ptr[position])) {
            exponent = exponent * 10 + (ptr[position] - '0');
            position++;
        }
    }

    VERIFY(position == length);

    // Combined calculation: sign * (integer + fractional/scale) * 10^(exponent_sign*exponent)
    const double base_value = integer_part + (fractional_part / fractional_scale);
    const double exponent_value = exponent_sign * exponent;
    return sign * base_value * pow(10, exponent_value);
}