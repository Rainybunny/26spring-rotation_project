absl::Status ClusterImplBase::parseDropOverloadConfig(
    const envoy::config::endpoint::v3::ClusterLoadAssignment& cluster_load_assignment) {
  // Default drop_overload_ to zero.
  drop_overload_ = UnitFloat(0);

  if (!cluster_load_assignment.has_policy()) {
    return absl::OkStatus();
  }
  auto policy = cluster_load_assignment.policy();
  if (policy.drop_overloads().empty()) {
    return absl::OkStatus();
  }
  if (policy.drop_overloads().size() > kDropOverloadSize) {
    return absl::InvalidArgumentError(
        fmt::format("Cluster drop_overloads config has {} categories. Envoy only support one.",
                    policy.drop_overloads().size()));
  }

  const auto drop_percentage = policy.drop_overloads(0).drop_percentage();
  // Lookup table for denominators
  static constexpr float denominators[] = {100.0f, 10000.0f, 1000000.0f};
  const auto denominator_type = drop_percentage.denominator();
  if (denominator_type < envoy::type::v3::FractionalPercent::HUNDRED || 
      denominator_type > envoy::type::v3::FractionalPercent::MILLION) {
    return absl::InvalidArgumentError(fmt::format(
        "Cluster drop_overloads config denominator setting is invalid : {}. Valid range 0~2.",
        denominator_type));
  }
  drop_overload_ = UnitFloat(float(drop_percentage.numerator()) / 
                            denominators[denominator_type]);
  return absl::OkStatus();
}