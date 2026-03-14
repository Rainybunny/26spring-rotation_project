SPDLOG_INLINE spdlog::level::level_enum from_str(const std::string &name) SPDLOG_NOEXCEPT
{
    // check special cases first since they're common
    if (name == "warn") return level::warn;
    if (name == "err") return level::err;

    // unrolled loop for main level names
    const auto size = sizeof(level_string_views) / sizeof(level_string_views[0]);
    for (size_t i = 0; i < size; ++i)
    {
        if (level_string_views[i] == name)
        {
            return static_cast<level::level_enum>(i);
        }
    }
    return level::off;
}