std::string GetOneInputCode(const OperationType& op_type,
                            CalculationsPrecision precision,
                            const std::string& input0) {
  if (input0 == "in_out_value") {
    switch (op_type) {
      case OperationType::ABS:
        return "in_out_value = fabs(in_out_value);\n";
      case OperationType::COS:
        return "in_out_value = cos(in_out_value);\n";
      case OperationType::COPY:
        return "\n";
      case OperationType::ELU:
        return "in_out_value.x = in_out_value.x < (FLT)(0.0f) ? exp(in_out_value.x) - (FLT)(1.0f) : in_out_value.x;\n"
               "in_out_value.y = in_out_value.y < (FLT)(0.0f) ? exp(in_out_value.y) - (FLT)(1.0f) : in_out_value.y;\n"
               "in_out_value.z = in_out_value.z < (FLT)(0.0f) ? exp(in_out_value.z) - (FLT)(1.0f) : in_out_value.z;\n"
               "in_out_value.w = in_out_value.w < (FLT)(0.0f) ? exp(in_out_value.w) - (FLT)(1.0f) : in_out_value.w;\n";
      case OperationType::EXP:
        return "in_out_value = exp(in_out_value);\n";
      case OperationType::HARD_SWISH:
        return "in_out_value *= clamp(in_out_value * (FLT)(0.16666667f) + (FLT)(0.5f), (FLT4)(0.0f), (FLT4)(1.0f));\n";
      case OperationType::LOG:
        return "in_out_value = log(in_out_value);\n";
      case OperationType::RSQRT:
        return "in_out_value = (FLT4)(1.0f) / sqrt(in_out_value);\n";
      case OperationType::SIGMOID:
        if (precision != CalculationsPrecision::F32) {
          return "in_out_value.x = convert_half(native_recip(1.0f + native_exp(convert_float(-in_out_value.x))));\n"
                 "in_out_value.y = convert_half(native_recip(1.0f + native_exp(convert_float(-in_out_value.y))));\n"
                 "in_out_value.z = convert_half(native_recip(1.0f + native_exp(convert_float(-in_out_value.z))));\n"
                 "in_out_value.w = convert_half(native_recip(1.0f + native_exp(convert_float(-in_out_value.w))));\n";
        } else {
          return "in_out_value = (FLT4)(1.0f) / ((FLT4)(1.0f) + exp(-(in_out_value)));\n";
        }
      case OperationType::SIN:
        return "in_out_value = sin(in_out_value);\n";
      case OperationType::SQRT:
        return "in_out_value = sqrt(in_out_value);\n";
      case OperationType::SQUARE:
        return "in_out_value *= in_out_value;\n";
      case OperationType::TANH:
        return "in_out_value = tanh(in_out_value);\n";
      default:
        return "Unknown operation type;\n";
    }
  }

  // Fallback for non-standard input0
  std::string result;
  switch (op_type) {
    case OperationType::ABS:
      result = "$0 = fabs($0);\n";
      break;
    case OperationType::COS:
      result = "$0 = cos($0);\n";
      break;
    case OperationType::COPY:
      result = "\n";
      break;
    case OperationType::ELU:
      result = "$0.x = $0.x < (FLT)(0.0f) ? exp($0.x) - (FLT)(1.0f) : $0.x;\n";
      result += "$0.y = $0.y < (FLT)(0.0f) ? exp($0.y) - (FLT)(1.0f) : $0.y;\n";
      result += "$0.z = $0.z < (FLT)(0.0f) ? exp($0.z) - (FLT)(1.0f) : $0.z;\n";
      result += "$0.w = $0.w < (FLT)(0.0f) ? exp($0.w) - (FLT)(1.0f) : $0.w;\n";
      break;
    case OperationType::EXP:
      result = "$0 = exp($0);\n";
      break;
    case OperationType::HARD_SWISH:
      result = "$0 *= clamp($0 * (FLT)(0.16666667f) + (FLT)(0.5f), (FLT4)(0.0f), (FLT4)(1.0f));\n";
      break;
    case OperationType::LOG:
      result = "$0 = log($0);\n";
      break;
    case OperationType::RSQRT:
      result = "$0 = (FLT4)(1.0f) / sqrt($0);\n";
      break;
    case OperationType::SIGMOID:
      if (precision != CalculationsPrecision::F32) {
        result = "$0.x = convert_half(native_recip(1.0f + native_exp(convert_float(-$0.x))));\n";
        result += "$0.y = convert_half(native_recip(1.0f + native_exp(convert_float(-$0.y))));\n";
        result += "$0.z = convert_half(native_recip(1.0f + native_exp(convert_float(-$0.z))));\n";
        result += "$0.w = convert_half(native_recip(1.0f + native_exp(convert_float(-$0.w))));\n";
      } else {
        result = "$0 = (FLT4)(1.0f) / ((FLT4)(1.0f) + exp(-($0)));\n";
      }
      break;
    case OperationType::SIN:
      result = "$0 = sin($0);\n";
      break;
    case OperationType::SQRT:
      result = "$0 = sqrt($0);\n";
      break;
    case OperationType::SQUARE:
      result = "$0 *= $0;\n";
      break;
    case OperationType::TANH:
      result = "$0 = tanh($0);\n";
      break;
    default:
      return "Unknown operation type;\n";
  }
  return absl::Substitute(result, input0);
}