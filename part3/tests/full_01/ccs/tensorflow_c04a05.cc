// Allows the host to be a raw IP (either v4 or v6).
Status ValidateHostPortPair(const string& host_port) {
  static const string bns_prefix("/bns/");
  const size_t host_port_len = host_port.length();
  const size_t bns_prefix_len = bns_prefix.length();
  
  // Fast path for BNS prefix
  if (host_port_len >= bns_prefix_len && 
      host_port.compare(0, bns_prefix_len, bns_prefix) == 0) {
    return Status::OK();
  }

  // Port validation
  const size_t colon_index = host_port.find_last_of(':');
  if (colon_index == string::npos || colon_index + 1 >= host_port_len) {
    return errors::InvalidArgument("Could not interpret \"", host_port,
                                 "\" as a host-port pair.");
  }

  uint32 port;
  if (!strings::safe_strtou32(host_port.substr(colon_index + 1), &port) ||
      host_port.find('/', 0, colon_index) != string::npos) {
    return errors::InvalidArgument("Could not interpret \"", host_port,
                                 "\" as a host-port pair.");
  }
  return Status::OK();
}