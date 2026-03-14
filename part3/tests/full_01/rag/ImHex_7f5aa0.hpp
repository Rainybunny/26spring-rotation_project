template<typename ... Args>
inline std::string format(const std::string &format, Args ... args) {
    // First try with stack buffer
    char stackBuffer[256];
    ssize_t size = snprintf(stackBuffer, sizeof(stackBuffer), format.c_str(), args...);
    
    if (size < 0) return "";
    
    // If it fits in stack buffer, construct directly
    if (size < static_cast<ssize_t>(sizeof(stackBuffer)))
        return std::string(stackBuffer, size);
    
    // Otherwise fall back to original two-pass approach
    size = snprintf(nullptr, 0, format.c_str(), args...);
    if (size <= 0) return "";
    
    std::vector<char> buffer(size + 1);
    snprintf(buffer.data(), size + 1, format.c_str(), args...);
    
    return std::string(buffer.data(), buffer.data() + size);
}