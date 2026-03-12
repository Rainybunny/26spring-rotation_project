SPDLOG_INLINE spdlog::level::level_enum from_str(const std::string &name) SPDLOG_NOEXCEPT
{
    // check first for common cases "warn" and "err" before linear search
    if (name == "warn")
    {
        return level::warn;
    }
    if (name == "err")
    {
        return level::err;
    }

    auto it = std::find(std::begin(level_string_views), std::end(level_string_views), name);
    if (it != std::end(level_string_views))
        return static_cast<level::level_enum>(std::distance(std::begin(level_string_views), it));

    return level::off;
}