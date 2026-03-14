bool Utility::isUpgrade(const RequestOrResponseHeaderMap& headers) {
  if (!headers.Upgrade()) {
    return false;
  }

  absl::string_view connection = headers.getConnectionValue();
  if (connection.empty()) {
    return false;
  }

  // Fast path for exact match
  const auto& upgrade = Http::Headers::get().ConnectionValues.Upgrade;
  if (connection.size() == upgrade.size() &&
      absl::EqualsIgnoreCase(connection, upgrade)) {
    return true;
  }

  // Common case: "keep-alive, Upgrade" or similar
  if (connection.size() > upgrade.size()) {
    size_t pos = connection.rfind(',');
    if (pos != absl::string_view::npos) {
      absl::string_view last_token = connection.substr(pos + 1);
      last_token = Envoy::StringUtil::trim(last_token);
      if (absl::EqualsIgnoreCase(last_token, upgrade)) {
        return true;
      }
    }
  }

  // Fallback to full tokenization if needed
  return Envoy::StringUtil::caseFindToken(connection, ",", upgrade.c_str());
}