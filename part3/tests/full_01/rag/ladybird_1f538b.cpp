// https://www.w3.org/TR/css-syntax-3/#convert-string-to-number
double Tokenizer::convert_a_string_to_a_number(StringView string)
{
    // Precomputed powers of 10 for common fractional digit counts (0-20)
    static constexpr double pow10_neg[] = {
        1.0,
        0.1,
        0.01,
        0.001,
        0.0001,
        0.00001,
        0.000001,
        0.0000001,
        0.00000001,
        0.000000001,
        0.0000000001,
        0.00000000001,
        0.000000000001,
        0.0000000000001,
        0.00000000000001,
        0.000000000000001,
        0.0000000000000001,
        0.00000000000000001,
        0.000000000000000001,
        0.0000000000000000001,
        0.00000000000000000001
    };

    const char* ptr = string.characters_without_null_termination();
    const char* end = ptr + string.length();

    // 1. Handle sign
    int sign = 1;
    if (ptr < end && (*ptr == '+' || *ptr == '-')) {
        sign = (*ptr == '-') ? -1 : 1;
        ptr++;
    }

    // 2. Integer part
    double integer_part = 0;
    while (ptr < end && is_ascii_digit(*ptr)) {
        integer_part = integer_part * 10 + (*ptr - '0');
        ptr++;
    }

    // 3. Decimal point
    if (ptr < end && *ptr == '.')
        ptr++;

    // 4. Fractional part
    double fractional_part = 0;
    int fractional_digits = 0;
    while (ptr < end && is_ascii_digit(*ptr)) {
        fractional_part = fractional_part * 10 + (*ptr - '0');
        fractional_digits++;
        ptr++;
    }

    // 5. Exponent indicator
    if (ptr < end && (*ptr == 'e' || *ptr == 'E'))
        ptr++;

    // 6. Exponent sign
    int exponent_sign = 1;
    if (ptr < end && (*ptr == '+' || *ptr == '-')) {
        exponent_sign = (*ptr == '-') ? -1 : 1;
        ptr++;
    }

    // 7. Exponent
    double exponent = 0;
    while (ptr < end && is_ascii_digit(*ptr)) {
        exponent = exponent * 10 + (*ptr - '0');
        ptr++;
    }

    // Calculate result using precomputed powers where possible
    double fractional_multiplier = (fractional_digits >= 0 && fractional_digits <= 20) 
        ? pow10_neg[fractional_digits] 
        : pow(10, -fractional_digits);
    
    double exponent_multiplier = pow(10, exponent_sign * exponent);

    return sign * (integer_part + fractional_part * fractional_multiplier) * exponent_multiplier;
}