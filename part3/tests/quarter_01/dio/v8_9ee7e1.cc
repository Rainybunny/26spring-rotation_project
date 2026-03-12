static Object* Runtime_Math_random(Arguments args) {
  NoHandleAllocation ha;
  ASSERT(args.length() == 0);

  // Use xorshift128+ PRNG for better performance
  static uint64_t state[2] = {1, 0};
  uint64_t x = state[0];
  uint64_t const y = state[1];
  state[0] = y;
  x ^= x << 23;
  state[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
  uint64_t random_bits = state[1] + y;

  // Convert to double in [0,1) range by using 52 high bits (mantissa bits)
  double result = (random_bits >> 12) * 0x1.0p-52;
  ASSERT(result >= 0 && result < 1);
  return Heap::AllocateHeapNumber(result);
}