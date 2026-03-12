SPDLOG_INLINE spdlog::level::level_enum from_str(const std::string &name) SPDLOG_NOEXCEPT
{
    // Check most common cases first with length and memcmp
    const size_t len = name.size();
    if (len == 4)
    {
        if (memcmp(name.data(), "warn", 4) == 0)
        {
            return level::warn;
        }
        if (memcmp(name.data(), "info", 4) == 0)
        {
            return level::info;
        }
    }
    else if (len == 3)
    {
        if (memcmp(name.data(), "err", 3) == 0)
        {
            return level::err;
        }
    }

    // Fallback to linear search for less common levels
    auto it = std::find(std::begin(level_string_views), std::end(level_string_views), name);
    if (it != std::end(level_string_views))
    {
        return static_cast<level::level_enum>(std::distance(std::begin(level_string_views), it));
    }

    return level::off;
}