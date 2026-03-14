template<typename ... Args>
    inline std::string format(const std::string &format, Args ... args) {
        // Try stack buffer first for small strings
        char stackBuffer[256];
        ssize_t size = snprintf(stackBuffer, sizeof(stackBuffer), format.c_str(), args...);
        
        if (size < 0) return "";
        
        // If it fits in stack buffer, construct directly
        if (size < static_cast<ssize_t>(sizeof(stackBuffer)))
            return std::string(stackBuffer, size);
            
        // Otherwise fall back to original dynamic allocation
        std::vector<char> heapBuffer(size + 1);
        snprintf(heapBuffer.data(), size + 1, format.c_str(), args...);
        return std::string(heapBuffer.data(), size);
    }