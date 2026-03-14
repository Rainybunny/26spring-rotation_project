static Object* Runtime_Math_random(Arguments args) {
  NoHandleAllocation ha;
  ASSERT(args.length() == 0);

  // Use thread-local state for rand_r
  static __thread unsigned int seed = 0;
  if (seed == 0) {
    // Initialize seed if not already done
    seed = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(&seed));
    seed ^= static_cast<unsigned int>(OS::Ticks());
  }

  // Get random bits - rand_r is generally faster than random()
  const double kInvRandMax = 1.0 / (RAND_MAX + 1.0);
  double result = rand_r(&seed) * kInvRandMax;
  ASSERT(result >= 0 && result < 1);
  return Heap::AllocateHeapNumber(result);
}