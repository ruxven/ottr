#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <string>

namespace ottr {

using Tick = long long; // tenths of an hour

enum class RoundingMode {
  Truncate,  // floor division (default)
  Round,     // standard rounding
  Ceil       // ceiling division
};

enum class AggregationMode {
  PerEvent,  // allocate each slice immediately (default)
  PerDay,    // accumulate slices, allocate at end of day
  PerMonth,  // accumulate slices, allocate at end of month
  Off        // explicit no-op (same as PerEvent)
};

enum class RemainderPolicy {
  Priority,    // priority, budget, fractional part, then id (default)
  RoundRobin,  // cycle through eligible charges
  LargestFrac  // fractional part first, then priority
};

enum class BudgetPolicy {
  Ignore,  // current behavior; budgets are only a tie-break (default)
  Warn,    // allocate normally and report each overrun
  Cap,     // stop allocating to a charge at its budget
  Spill    // cap every charge, send the remainder to overflow_charge
};

struct CalculationOptions {
  RoundingMode rounding = RoundingMode::Truncate;
  AggregationMode aggregation = AggregationMode::PerEvent;
  RemainderPolicy remainder = RemainderPolicy::Priority;
  BudgetPolicy budget = BudgetPolicy::Ignore;
  std::string overflow_charge;  // required by, and only meaningful for, Spill
  Tick granularity = 1;         // charge quantum in ticks; 1 = 0.1 hour
  bool audit = false;           // record per-allocation detail

  // Validate options and return error message if invalid
  bool validate(std::string& error_message) const;

  // Parse rounding mode from string
  static bool parse_rounding(const std::string& s, RoundingMode& out,
                             std::string& error_message);

  // Parse aggregation mode from string
  static bool parse_aggregation(const std::string& s, AggregationMode& out,
                                std::string& error_message);

  // Parse remainder policy from string
  static bool parse_remainder(const std::string& s, RemainderPolicy& out,
                              std::string& error_message);

  // Parse budget policy from string
  static bool parse_budget(const std::string& s, BudgetPolicy& out,
                           std::string& error_message);

  // Parse a granularity in decimal hours into ticks; must be a positive
  // multiple of 0.1 hour.
  static bool parse_granularity(const std::string& s, Tick& out,
                                std::string& error_message);

  // Parse an on/off flag
  static bool parse_flag(const std::string& s, bool& out,
                         std::string& error_message);
};

}  // namespace ottr

#endif  // OPTIONS_HPP
