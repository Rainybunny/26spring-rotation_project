bool Utility::isUpgrade(const RequestOrResponseHeaderMap& headers) {
  if (!headers.Upgrade()) {
    return false;
  }

  absl::string_view connection = headers.getConnectionValue();
  if (connection.empty()) {
    return false;
  }

  // Fast path for common case where Connection is exactly "Upgrade"
  if (absl::EqualsIgnoreCase(connection, Http::Headers::get().ConnectionValues.Upgrade)) {
    return true;
  }

  // Only do full token search if comma is present
  if (connection.find(',') != absl::string_view::npos) {
    return Envoy::StringUtil::caseFindToken(connection, ",",
                                          Http::Headers::get().ConnectionValues.Upgrade.c_str());
  }

  return false;
}