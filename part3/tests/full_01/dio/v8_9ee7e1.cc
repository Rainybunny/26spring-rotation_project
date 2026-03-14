static Object* Runtime_Math_random(Arguments args) {
  NoHandleAllocation ha;
  ASSERT(args.length() == 0);

  // Cache the reciprocal as multiplication is faster than division
  static const double kReciprocalRandMaxPlus1 = 1.0 / (RAND_MAX + 1.0);
  
  // Get a single random value and use its bits more efficiently
  uint32_t random_val = static_cast<uint32_t>(random());
  
  // Use high bits for the main value and low bits for extra precision
  double result = (random_val >> 8) * kReciprocalRandMaxPlus1;
  result += (random_val & 0xFF) * (kReciprocalRandMaxPlus1 * (1.0 / 256.0));
  
  ASSERT(result >= 0 && result < 1);
  return Heap::AllocateHeapNumber(result);
}