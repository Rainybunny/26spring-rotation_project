static Object* Runtime_Math_random(Arguments args) {
  NoHandleAllocation ha;
  ASSERT(args.length() == 0);

  // Use a simple and fast Xorshift PRNG with precomputed reciprocal
  static uint32_t state = 1;
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  
  // Convert to double in [0,1) range using precomputed reciprocal
  // of 2^32 to avoid division
  static const double kReciprocalU32 = 2.3283064365386963e-10; // 1/2^32
  double result = state * kReciprocalU32;
  ASSERT(result >= 0 && result < 1);
  return Heap::AllocateHeapNumber(result);
}