// https://www.w3.org/TR/css-syntax-3/#convert-string-to-number
double Tokenizer::convert_a_string_to_a_number(StringView string)
{
    // Precomputed powers of 10^-n for n = 0..20
    static constexpr double fractional_powers[] = {
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

    auto code_point_at = [&](size_t index) -> u32 {
        if (index < string.length())
            return string[index];
        return TOKENIZER_EOF;
    };

    size_t position = 0;

    int sign = 1;
    if (is_plus_sign(code_point_at(position)) || is_hyphen_minus(code_point_at(position))) {
        sign = is_hyphen_minus(code_point_at(position)) ? -1 : 1;
        position++;
    }

    double integer_part = 0;
    while (is_ascii_digit(code_point_at(position))) {
        integer_part = (integer_part * 10) + (code_point_at(position) - '0');
        position++;
    }

    if (is_full_stop(code_point_at(position)))
        position++;

    double fractional_part = 0;
    int fractional_digits = 0;
    while (is_ascii_digit(code_point_at(position))) {
        fractional_part = (fractional_part * 10) + (code_point_at(position) - '0');
        position++;
        fractional_digits++;
    }

    if (is_e(code_point_at(position)) || is_E(code_point_at(position)))
        position++;

    int exponent_sign = 1;
    if (is_plus_sign(code_point_at(position)) || is_hyphen_minus(code_point_at(position))) {
        exponent_sign = is_hyphen_minus(code_point_at(position)) ? -1 : 1;
        position++;
    }

    double exponent = 0;
    while (is_ascii_digit(code_point_at(position))) {
        exponent = (exponent * 10) + (code_point_at(position) - '0');
        position++;
    }

    VERIFY(position == string.length());

    // Use lookup table for fractional power if possible, otherwise fall back to pow()
    double fractional_power = (fractional_digits <= 20) 
        ? fractional_powers[fractional_digits] 
        : pow(10, -fractional_digits);

    return sign * (integer_part + fractional_part * fractional_power) * pow(10, exponent_sign * exponent);
}