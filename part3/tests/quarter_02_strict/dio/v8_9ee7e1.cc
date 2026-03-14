static Object* Runtime_Math_random(Arguments args) {
  NoHandleAllocation ha;
  ASSERT(args.length() == 0);

  // Precompute reciprocal of (RAND_MAX + 1.0) for faster division
  static const double kReciprocalRandMaxPlusOne = 1.0 / (RAND_MAX + 1.0);
  
  // Single random call gives sufficient precision for most use cases
  double result = static_cast<double>(random()) * kReciprocalRandMaxPlusOne;
  ASSERT(result >= 0 && result < 1);
  return Heap::AllocateHeapNumber(result);
}