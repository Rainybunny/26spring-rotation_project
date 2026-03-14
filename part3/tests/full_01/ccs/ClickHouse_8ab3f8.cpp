DateLUT::DateLUT()
{
	/** Дополнительный вызов Time2Date для случая, когда в 1981-1984 году в России,
	  * 1 апреля начиналось в час ночи, не в полночь.
	  */
	for (time_t t = Time2Date(DATE_LUT_MIN), next_t = Time2Date(TimeDayShift(t));
		t <= DATE_LUT_MAX;
		t = next_t, next_t = Time2Date(TimeDayShift(t)))
	{
		Values values;

		struct tm tm;
		localtime_r(&t, &tm);

		const int tm_year = tm.tm_year;
		const int tm_mon = tm.tm_mon;
		const int tm_wday = tm.tm_wday;
		const int tm_mday = tm.tm_mday;

		values.year = tm_year + 1900;
		values.month = tm_mon + 1;
		values.day_of_week = tm_wday == 0 ? 7 : tm_wday;
		values.day_of_month = tm_mday;

		tm.tm_hour = 0;
		tm.tm_min = 0;
		tm.tm_sec = 0;
		tm.tm_isdst = -1;

		values.date = mktime(&tm);

		lut.push_back(values);
	}

	/// Заполняем lookup таблицу для годов
	memset(years_lut, 0, DATE_LUT_YEARS * sizeof(years_lut[0]));
	for (size_t day = 0, size = lut.size(); day < size && lut[day].year <= DATE_LUT_MAX_YEAR; ++day)
	{
		const Values& day_values = lut[day];
		if (day_values.month == 1 && day_values.day_of_month == 1)
			years_lut[day_values.year - DATE_LUT_MIN_YEAR] = day;
	}

	offset_at_start_of_epoch = 86400 - lut[findIndex(86400)].date;
}