// Allows the host to be a raw IP (either v4 or v6).
Status ValidateHostPortPair(const string& host_port) {
  uint32 port;
  auto colon_index = host_port.find_last_of(':');
  if (!strings::safe_strtou32(host_port.substr(colon_index + 1), &port) ||
      host_port.substr(0, colon_index).find('/') != string::npos) {
    // Check for BNS prefix only after port validation fails
    static const char bns_prefix[] = "/bns/";
    if (host_port.compare(0, sizeof(bns_prefix)-1, bns_prefix) == 0) {
      return Status::OK();
    }
    return errors::InvalidArgument("Could not interpret \"", host_port,
                                 "\" as a host-port pair.");
  }
  return Status::OK();
}