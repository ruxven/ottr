#pragma once
// See SPEC.txt and DESIGN.txt for detailed definitions.
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

namespace ottr {

using Tick = long long; // tenths of an hour

struct Date {
    std::string raw; // MM/DD
    int ord = 0;     // MM*100 + DD
};

struct ChargeNumber {
    std::string id;
    std::string description;
    Tick budget_ticks = 0;
    int priority = 0;
    std::map<std::string, Tick> charged_by_day; // date.raw -> ticks
    Tick total_charged_ticks = 0;
};

struct Task {
    std::string name;
    std::string description;
    std::unordered_map<std::string, int> weights; // charge_id -> weight
    bool declared = false; // true if introduced by a 'task' directive
};

enum class LogKind { None, Task, Charge };

struct Log {
    Tick time_ticks = 0; // time-of-day in ticks
    LogKind kind = LogKind::None;
    std::string ref_id; // task or charge id
    std::string description; // optional
    int line_no = 0;
};

struct Day {
    Date date;
    std::vector<Log> logs; // strictly increasing by time
};

struct World {
    std::unordered_map<std::string, ChargeNumber> charges; // id -> charge
    std::unordered_map<std::string, Task> tasks;            // name -> task
    std::vector<Day> days;                                  // in file order

    Day& current_day();
};

// Aggregation snapshot produced by processing and used by Reporter
struct Aggregation {
    // charge_id -> (date.raw -> ticks)
    std::unordered_map<std::string, std::unordered_map<std::string, Tick>> by_charge_by_day;
    // charge_id -> total ticks (within filter)
    std::unordered_map<std::string, Tick> totals_by_charge;
    // date.raw -> total charged ticks for the day
    std::unordered_map<std::string, Tick> totals_by_day;
    // ordered set of included dates
    std::vector<Date> dates_in_order;
};

// Date filter as interpreted by CLI
struct DateFilter {
    // Modes: none (all), single, range
    bool has_start = false;
    bool has_end = false;
    Date start{};
    Date end{};
    bool includes(const Date& d) const;
};

} // namespace ottr
