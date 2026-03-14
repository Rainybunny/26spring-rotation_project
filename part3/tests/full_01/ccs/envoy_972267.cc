bool Utility::isUpgrade(const RequestOrResponseHeaderMap& headers) {
  if (!headers.Upgrade()) {
    return false;
  }

  static const absl::string_view upgrade_token = Http::Headers::get().ConnectionValues.Upgrade;
  absl::string_view connection_value = headers.getConnectionValue();

  // Fast path for common case where connection header is exactly "Upgrade"
  if (absl::EqualsIgnoreCase(connection_value, upgrade_token)) {
    return true;
  }

  // Fall back to token parsing for more complex cases
  return Envoy::StringUtil::caseFindToken(connection_value, ",", upgrade_token);
}