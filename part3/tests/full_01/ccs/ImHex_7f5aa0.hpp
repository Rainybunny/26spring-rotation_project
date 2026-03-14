template<typename ... Args>
    inline std::string format(const std::string &format, Args ... args) {
        ssize_t size = snprintf(nullptr, 0, format.c_str(), args...);

        if (size <= 0)
            return "";

        std::string result;
        result.reserve(size + 1);
        result.resize(size);
        snprintf(result.data(), size + 1, format.c_str(), args...);

        return result;
    }