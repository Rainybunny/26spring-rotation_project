std::string GetOneInputCode(const OperationType& op_type,
                            CalculationsPrecision precision,
                            const std::string& input0) {
  std::string result;
  switch (op_type) {
    case OperationType::ABS:
      result.reserve(16);
      result = "$0 = fabs($0);\n";
      break;
    case OperationType::COS:
      result.reserve(16);
      result = "$0 = cos($0);\n";
      break;
    case OperationType::COPY:
      result.reserve(1);
      result = "\n";
      break;
    case OperationType::ELU:
      result.reserve(200);
      result = "$0.x = $0.x < (FLT)(0.0f) ? exp($0.x) - (FLT)(1.0f) : $0.x;\n";
      result.append("$0.y = $0.y < (FLT)(0.0f) ? exp($0.y) - (FLT)(1.0f) : $0.y;\n");
      result.append("$0.z = $0.z < (FLT)(0.0f) ? exp($0.z) - (FLT)(1.0f) : $0.z;\n");
      result.append("$0.w = $0.w < (FLT)(0.0f) ? exp($0.w) - (FLT)(1.0f) : $0.w;\n");
      break;
    case OperationType::EXP:
      result.reserve(16);
      result = "$0 = exp($0);\n";
      break;
    case OperationType::HARD_SWISH:
      result.reserve(100);
      result =
          "$0 *= clamp($0 * (FLT)(0.16666667f) + (FLT)(0.5f), (FLT4)(0.0f), "
          "(FLT4)(1.0f));\n";
      break;
    case OperationType::LOG:
      result.reserve(16);
      result = "$0 = log($0);\n";
      break;
    case OperationType::RSQRT:
      result.reserve(18);
      result = "$0 = rsqrt($0);\n";
      break;
    case OperationType::SIGMOID:
      if (precision != CalculationsPrecision::F32) {
        result.reserve(300);
        result =
            "$0.x = convert_half(native_recip(1.0f + "
            "native_exp(convert_float(-$0.x))));\n";
        result.append(
            "$0.y = convert_half(native_recip(1.0f + "
            "native_exp(convert_float(-$0.y))));\n");
        result.append(
            "$0.z = convert_half(native_recip(1.0f + "
            "native_exp(convert_float(-$0.z))));\n");
        result.append(
            "$0.w = convert_half(native_recip(1.0f + "
            "native_exp(convert_float(-$0.w))));\n");
      } else {
        result.reserve(100);
        result = "$0 = (FLT4)(1.0f) / ((FLT4)(1.0f) + exp(-($0)));\n";
      }
      break;
    case OperationType::SIN:
      result.reserve(16);
      result = "$0 = sin($0);\n";
      break;
    case OperationType::SQRT:
      result.reserve(16);
      result = "$0 = sqrt($0);\n";
      break;
    case OperationType::SQUARE:
      result.reserve(12);
      result = "$0 *= $0;\n";
      break;
    case OperationType::TANH:
      result.reserve(16);
      result = "$0 = tanh($0);\n";
      break;
    default:
      result.reserve(24);
      return "Unknown operation type;\n";
  }
  return absl::Substitute(result, input0);
}