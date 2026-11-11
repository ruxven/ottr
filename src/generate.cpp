#include "generate.hpp"
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>

namespace ottr {

namespace {

bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int days_in_month(int year, int month) {
    static const int days[] = {31, 28, 31, 30, 31, 30,
                               31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year)) return 29;
    return days[month - 1];
}

// Days since 1970-01-01. Howard Hinnant's civil calendar algorithms, which
// keep the date math independent of the C library's time zone handling.
long long days_from_civil(int year, int month, int day) {
    year -= month <= 2 ? 1 : 0;
    const long long era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = static_cast<unsigned>(
        (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1);
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long long>(doe) - 719468;
}

void civil_from_days(long long days, int& year, int& month, int& day) {
    days += 719468;
    const long long era = (days >= 0 ? days : days - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(days - era * 146097);
    const unsigned yoe =
        (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int y = static_cast<int>(yoe) + static_cast<int>(era) * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    day = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    month = static_cast<int>(mp + (mp < 10 ? 3 : -9));
    year = y + (month <= 2 ? 1 : 0);
}

// ISO weekday: 0 = Monday ... 6 = Sunday. 1970-01-01 was a Thursday.
int iso_weekday(long long days) {
    int wd = static_cast<int>((days + 3) % 7);
    if (wd < 0) wd += 7;
    return wd;
}

// ISO-8601 week number: week 1 is the week containing the first Thursday of
// the year, and weeks start on Monday.
int iso_week(int year, int month, int day) {
    const long long days = days_from_civil(year, month, day);
    const int wd = iso_weekday(days);
    const long long thursday = days - wd + 3;
    int ty = 0, tm = 0, td = 0;
    civil_from_days(thursday, ty, tm, td);
    const long long jan1 = days_from_civil(ty, 1, 1);
    return static_cast<int>((thursday - jan1) / 7) + 1;
}

int day_of_year(int year, int month, int day) {
    return static_cast<int>(days_from_civil(year, month, day) -
                            days_from_civil(year, 1, 1)) + 1;
}

std::string format_ymd(const CalendarDate& date) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << date.year << '/'
        << std::setw(2) << date.month << '/' << std::setw(2) << date.day;
    return oss.str();
}

std::string format_md(const CalendarDate& date) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << date.month << '/'
        << std::setw(2) << date.day;
    return oss.str();
}

bool all_digits(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

} // namespace

bool operator<(const CalendarDate& a, const CalendarDate& b) {
    if (a.year != b.year) return a.year < b.year;
    if (a.month != b.month) return a.month < b.month;
    return a.day < b.day;
}

bool operator<=(const CalendarDate& a, const CalendarDate& b) {
    return !(b < a);
}

bool parse_calendar_date(const std::string& s, int default_year,
                         CalendarDate& out) {
    // Split on '/'. Two fields mean MM/DD with default_year; three fields mean
    // yyyy/mm/dd.
    std::vector<std::string> parts;
    size_t begin = 0;
    while (true) {
        const size_t slash = s.find('/', begin);
        if (slash == std::string::npos) {
            parts.push_back(s.substr(begin));
            break;
        }
        parts.push_back(s.substr(begin, slash - begin));
        begin = slash + 1;
    }
    if (parts.size() != 2 && parts.size() != 3) return false;
    for (const std::string& part : parts) {
        if (!all_digits(part)) return false;
    }

    CalendarDate date;
    size_t index = 0;
    if (parts.size() == 3) {
        if (parts[0].size() != 4) return false; // yyyy
        date.year = std::atoi(parts[index++].c_str());
        if (date.year < 1 || date.year > 9999) return false;
    } else {
        date.year = default_year;
    }
    if (parts[index].size() > 2) return false; // MM
    date.month = std::atoi(parts[index++].c_str());
    if (parts[index].size() > 2) return false; // DD
    date.day = std::atoi(parts[index++].c_str());
    if (date.month < 1 || date.month > 12) return false;
    if (date.day < 1 || date.day > days_in_month(date.year, date.month)) {
        return false;
    }
    out = date;
    return true;
}

int current_year() {
    const std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    if (local == nullptr) return 1970;
    return local->tm_year + 1900;
}

bool parse_generate_range(const std::string& start_text,
                          const std::string& end_text, int default_year,
                          CalendarDate& start, CalendarDate& end,
                          std::string& error) {
    if (!parse_calendar_date(start_text, default_year, start)) {
        error = "invalid start date '" + start_text +
                "' (expected yyyy/mm/dd or mm/dd)";
        return false;
    }
    if (!parse_calendar_date(end_text, default_year, end)) {
        error = "invalid end date '" + end_text +
                "' (expected yyyy/mm/dd or mm/dd)";
        return false;
    }
    if (end < start) {
        error = "end date " + format_ymd(end) + " is before start date " +
                format_ymd(start);
        return false;
    }
    return true;
}

std::string generate_timecard(const CalendarDate& start,
                              const CalendarDate& end) {
    std::ostringstream oss;
    oss << "# " << format_ymd(start) << " - " << format_ymd(end) << "\n";

    const long long first = days_from_civil(start.year, start.month, start.day);
    const long long last = days_from_civil(end.year, end.month, end.day);
    for (long long days = first; days <= last; ++days) {
        int year = 0, month = 0, day = 0;
        civil_from_days(days, year, month, day);
        const int weekday = iso_weekday(days); // 0 = Monday
        if (weekday >= 5) continue;             // skip Saturday and Sunday

        CalendarDate date;
        date.year = year;
        date.month = month;
        date.day = day;

        oss << "\n";
        if (weekday == 0) {
            oss << "# week " << iso_week(year, month, day) << "\n\n";
        }
        oss << "# day " << day_of_year(year, month, day) << "\n"
            << "day " << format_md(date) << "\n"
            << "log 9.0\n"
            << "log 17.0\n";
    }
    return oss.str();
}

} // namespace ottr
