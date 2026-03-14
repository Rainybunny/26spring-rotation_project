DateLUT::DateLUT()
{
    /** Дополнительный вызов Time2Date для случая, когда в 1981-1984 году в России,
      * 1 апреля начиналось в час ночи, не в полночь.
      */
    time_t current = Time2Date(DATE_LUT_MIN);
    time_t end = DATE_LUT_MAX;
    
    // Precompute timezone info once
    struct tm tm_cache;
    localtime_r(&current, &tm_cache);
    const int timezone_offset = tm_cache.tm_gmtoff;
    
    while (current <= end)
    {
        Values values;
        struct tm tm;

        // Reuse cached timezone info
        tm = tm_cache;
        localtime_r(&current, &tm);

        values.year = tm.tm_year + 1900;
        values.month = tm.tm_mon + 1;
        values.day_of_week = tm.tm_wday == 0 ? 7 : tm.tm_wday;
        values.day_of_month = tm.tm_mday;

        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        tm.tm_isdst = -1;
        tm.tm_gmtoff = timezone_offset; // Use cached offset

        values.date = mktime(&tm);

        lut.push_back(values);
        current = Time2Date(TimeDayShift(current));
    }

    /// Заполняем lookup таблицу для годов
    memset(years_lut, 0, DATE_LUT_YEARS * sizeof(years_lut[0]));
    for (size_t day = 0; day < lut.size() && lut[day].year <= DATE_LUT_MAX_YEAR; ++day)
    {
        if (lut[day].month == 1 && lut[day].day_of_month == 1)
            years_lut[lut[day].year - DATE_LUT_MIN_YEAR] = day;
    }

    offset_at_start_of_epoch = 86400 - lut[findIndex(86400)].date;
}