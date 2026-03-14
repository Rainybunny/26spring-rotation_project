absl::Status ClusterImplBase::parseDropOverloadConfig(
    const envoy::config::endpoint::v3::ClusterLoadAssignment& cluster_load_assignment) {
  // Default drop_overload_ to zero.
  drop_overload_ = UnitFloat(0);

  if (!cluster_load_assignment.has_policy()) {
    return absl::OkStatus();
  }
  const auto& policy = cluster_load_assignment.policy();
  if (policy.drop_overloads().empty()) {
    return absl::OkStatus();
  }
  if (policy.drop_overloads().size() > kDropOverloadSize) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Cluster drop_overloads config has ", policy.drop_overloads().size(),
        " categories. Envoy only supports one."));
  }

  // Precomputed denominator values
  static constexpr float kDenominators[] = {100.0f, 10000.0f, 1000000.0f};
  const auto& drop_percentage = policy.drop_overloads(0).drop_percentage();
  const auto denominator_type = drop_percentage.denominator();

  if (denominator_type < envoy::type::v3::FractionalPercent::HUNDRED ||
      denominator_type > envoy::type::v3::FractionalPercent::MILLION) {
    return absl::InvalidArgumentError(
        "Cluster drop_overloads config denominator setting is invalid. Valid range 0~2.");
  }

  drop_overload_ = UnitFloat(drop_percentage.numerator() / 
                           kDenominators[denominator_type - 1]);
  return absl::OkStatus();
}