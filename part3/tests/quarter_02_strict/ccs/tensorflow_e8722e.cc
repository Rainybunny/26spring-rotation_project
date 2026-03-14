std::string GetOneInputCode(const OperationType& op_type,
                            CalculationsPrecision precision,
                            const std::string& input0) {
  std::string result;
  switch (op_type) {
    case OperationType::ABS:
      result = input0 + " = fabs(" + input0 + ");\n";
      break;
    case OperationType::COS:
      result = input0 + " = cos(" + input0 + ");\n";
      break;
    case OperationType::COPY:
      // No op as inout_value will be copied to dest automatically.
      result = "\n";
      break;
    case OperationType::ELU:
      result = input0 + ".x = " + input0 + ".x < (FLT)(0.0f) ? exp(" + input0 + ".x) - (FLT)(1.0f) : " + input0 + ".x;\n";
      result += input0 + ".y = " + input0 + ".y < (FLT)(0.0f) ? exp(" + input0 + ".y) - (FLT)(1.0f) : " + input0 + ".y;\n";
      result += input0 + ".z = " + input0 + ".z < (FLT)(0.0f) ? exp(" + input0 + ".z) - (FLT)(1.0f) : " + input0 + ".z;\n";
      result += input0 + ".w = " + input0 + ".w < (FLT)(0.0f) ? exp(" + input0 + ".w) - (FLT)(1.0f) : " + input0 + ".w;\n";
      break;
    case OperationType::EXP:
      result = input0 + " = exp(" + input0 + ");\n";
      break;
    case OperationType::HARD_SWISH:
      result = input0 + " *= clamp(" + input0 + " * (FLT)(0.16666667f) + (FLT)(0.5f), (FLT4)(0.0f), (FLT4)(1.0f));\n";
      break;
    case OperationType::LOG:
      result = input0 + " = log(" + input0 + ");\n";
      break;
    case OperationType::RSQRT:
      result = input0 + " = rsqrt(" + input0 + ");\n";
      break;
    case OperationType::SIGMOID:
      if (precision != CalculationsPrecision::F32) {
        result = input0 + ".x = convert_half(native_recip(1.0f + native_exp(convert_float(-" + input0 + ".x))));\n";
        result += input0 + ".y = convert_half(native_recip(1.0f + native_exp(convert_float(-" + input0 + ".y))));\n";
        result += input0 + ".z = convert_half(native_recip(1.0f + native_exp(convert_float(-" + input0 + ".z))));\n";
        result += input0 + ".w = convert_half(native_recip(1.0f + native_exp(convert_float(-" + input0 + ".w))));\n";
      } else {
        result = input0 + " = (FLT4)(1.0f) / ((FLT4)(1.0f) + exp(-(" + input0 + ")));\n";
      }
      break;
    case OperationType::SIN:
      result = input0 + " = sin(" + input0 + ");\n";
      break;
    case OperationType::SQRT:
      result = input0 + " = sqrt(" + input0 + ");\n";
      break;
    case OperationType::SQUARE:
      result = input0 + " *= " + input0 + ";\n";
      break;
    case OperationType::TANH:
      result = input0 + " = tanh(" + input0 + ");\n";
      break;
    default:
      return "Unknown operation type;\n";
  }
  return result;
}