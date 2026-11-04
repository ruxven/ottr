#pragma once
// See SPEC.txt and DESIGN.txt for detailed definitions.
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include "options.hpp"

namespace ottr {

struct Date {
    std::string raw; // MM/DD
    int ord = 0;     // MM*100 + DD
};

// How an allocation came out of a weighted split. The exact share is the
// ideal proportional share before any rounding; the rounded base is what the
// rounding mode produced; the adjustment is the net number of quanta the
// remainder policy moved between charges. The audit trail renders these so
// rounding effects are visible instead of silent.
struct RoundingDetail {
    bool applicable = false;   // true when the numbers below describe a split
    long long ideal_numer = 0; // exact share numerator, in ticks * weight units
    long long ideal_denom = 1; // exact share denominator (total weight); 1 = whole ticks
    Tick rounded_ticks = 0;    // base allocation after the rounding mode
    Tick adjust_ticks = 0;     // net quanta added (+) or removed (-) by remainder distribution
    Tick clamped_ticks = 0;    // ticks dropped by a budget cap
};

// One allocation, recorded when opt audit on.
struct AuditEntry {
    Date date;
    std::string note;      // "direct", "task", or "spill"
    std::string source;    // log ref: task name or charge id
    std::string charge_id; // charge the ticks landed on
    Tick ticks = 0;
    RoundingDetail rounding; // how the ticks were derived (see RoundingDetail)
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
    CalculationOptions options;                             // calculation configuration

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
    // per-allocation detail, populated only when opt audit is on
    std::vector<AuditEntry> audit;
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
