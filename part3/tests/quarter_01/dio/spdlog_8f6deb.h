SPDLOG_INLINE spdlog::level::level_enum from_str(const std::string &name) SPDLOG_NOEXCEPT
{
    // handle common special cases first
    if (name == "warn") return level::warn;
    if (name == "err") return level::err;

    // direct array access with early exit
    const auto count = sizeof(level_string_views) / sizeof(level_string_views[0]);
    for (size_t i = 0; i < count; ++i)
    {
        if (level_string_views[i] == name)
        {
            return static_cast<level::level_enum>(i);
        }
    }
    return level::off;
}