SPDLOG_INLINE spdlog::level::level_enum from_str(const std::string &name) SPDLOG_NOEXCEPT
{
    // Check special cases first since they're common
    if (name.size() == 4 && memcmp(name.data(), "warn", 4) == 0)
    {
        return level::warn;
    }
    if (name.size() == 3 && memcmp(name.data(), "err", 3) == 0)
    {
        return level::err;
    }

    // Main search with length check optimization
    const size_t length = name.size();
    for (size_t i = 0; i < sizeof(level_string_views)/sizeof(level_string_views[0]); ++i)
    {
        if (level_string_views[i].size() == length && 
            memcmp(level_string_views[i].data(), name.data(), length) == 0)
        {
            return static_cast<level::level_enum>(i);
        }
    }

    return level::off;
}