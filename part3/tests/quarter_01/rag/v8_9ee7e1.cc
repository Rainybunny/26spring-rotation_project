static Object* Runtime_Math_random(Arguments args) {
  NoHandleAllocation ha;
  ASSERT(args.length() == 0);

  // Precompute constants
  static const double kReciprocal = 1.0 / (RAND_MAX + 1.0);
  static const int kHalfBits = sizeof(random()) * 4; // Half the bits in random()

  // Get single random value and split into hi and lo parts
  uint32_t r = random();
  double hi = static_cast<double>(r >> kHalfBits);
  double lo = static_cast<double>(r & ((1 << kHalfBits) - 1)) * kReciprocal;

  // Combine and scale
  double result = (hi * kReciprocal + lo) * kReciprocal;
  ASSERT(result >= 0 && result < 1);
  return Heap::AllocateHeapNumber(result);
}