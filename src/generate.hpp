#pragma once
#include <string>

namespace ottr {

// A calendar date with a year, used only by the timecard generator. The rest
// of ottr keeps dates as MM/DD because a timecard has no year.
struct CalendarDate {
    int year = 0;
    int month = 0;
    int day = 0;
};

bool operator<(const CalendarDate& a, const CalendarDate& b);
bool operator<=(const CalendarDate& a, const CalendarDate& b);

// Parse "yyyy/mm/dd" or "mm/dd". For "mm/dd", default_year supplies the year.
bool parse_calendar_date(const std::string& s, int default_year,
                         CalendarDate& out);

// Current local year, used when a generation date omits the year.
int current_year();

// Parse the two --generate arguments and check that the range is ordered.
bool parse_generate_range(const std::string& start_text,
                          const std::string& end_text, int default_year,
                          CalendarDate& start, CalendarDate& end,
                          std::string& error);

// Generate a timecard skeleton from start through end, inclusive. Saturdays
// and Sundays are skipped. Each weekday gets a day block, Mondays also get an
// ISO week comment, and every block gets a day-of-year comment.
std::string generate_timecard(const CalendarDate& start,
                              const CalendarDate& end);

} // namespace ottr
