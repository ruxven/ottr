#include "options.hpp"
#include "util.hpp"
#include <cmath>

namespace ottr {

namespace {

const char* kValidRounding = "truncate, round, ceil";
const char* kValidAggregation = "per_event, per_day, per_month, off";
const char* kValidRemainder = "priority, round_robin, largest_frac";
const char* kValidBudget = "ignore, warn, cap, spill";

bool in_range(RoundingMode mode) {
  return mode == RoundingMode::Truncate || mode == RoundingMode::Round ||
         mode == RoundingMode::Ceil;
}

bool in_range(AggregationMode mode) {
  return mode == AggregationMode::PerEvent || mode == AggregationMode::PerDay ||
         mode == AggregationMode::PerMonth || mode == AggregationMode::Off;
}

bool in_range(RemainderPolicy policy) {
  return policy == RemainderPolicy::Priority ||
         policy == RemainderPolicy::RoundRobin ||
         policy == RemainderPolicy::LargestFrac;
}

bool in_range(BudgetPolicy policy) {
  return policy == BudgetPolicy::Ignore || policy == BudgetPolicy::Warn ||
         policy == BudgetPolicy::Cap || policy == BudgetPolicy::Spill;
}

} // namespace

bool CalculationOptions::validate(std::string& error_message) const {
  if (!in_range(rounding)) {
    error_message = "Invalid rounding mode value";
    return false;
  }
  if (!in_range(aggregation)) {
    error_message = "Invalid aggregation mode value";
    return false;
  }
  if (!in_range(remainder)) {
    error_message = "Invalid remainder policy value";
    return false;
  }
  if (!in_range(budget)) {
    error_message = "Invalid budget policy value";
    return false;
  }
  if (granularity <= 0) {
    error_message = "granularity must be a positive number of hours";
    return false;
  }
  if (budget == BudgetPolicy::Spill && overflow_charge.empty()) {
    error_message = "budget policy 'spill' requires opt overflow_charge <id>";
    return false;
  }
  if (budget != BudgetPolicy::Spill && !overflow_charge.empty()) {
    error_message = "opt overflow_charge is only valid with opt budget spill";
    return false;
  }
  return true;
}

bool CalculationOptions::parse_rounding(const std::string& s,
                                        RoundingMode& out,
                                        std::string& error_message) {
  if (s == "truncate") {
    out = RoundingMode::Truncate;
    return true;
  } else if (s == "round") {
    out = RoundingMode::Round;
    return true;
  } else if (s == "ceil") {
    out = RoundingMode::Ceil;
    return true;
  } else {
    error_message = "Invalid rounding mode: '" + s +
                    "'. Valid values: " + kValidRounding;
    return false;
  }
}

bool CalculationOptions::parse_aggregation(const std::string& s,
                                           AggregationMode& out,
                                           std::string& error_message) {
  if (s == "per_event") {
    out = AggregationMode::PerEvent;
    return true;
  } else if (s == "per_day") {
    out = AggregationMode::PerDay;
    return true;
  } else if (s == "per_month") {
    out = AggregationMode::PerMonth;
    return true;
  } else if (s == "off") {
    out = AggregationMode::Off;
    return true;
  } else {
    error_message = "Invalid aggregation mode: '" + s +
                    "'. Valid values: " + kValidAggregation;
    return false;
  }
}

bool CalculationOptions::parse_remainder(const std::string& s,
                                         RemainderPolicy& out,
                                         std::string& error_message) {
  if (s == "priority") {
    out = RemainderPolicy::Priority;
    return true;
  } else if (s == "round_robin") {
    out = RemainderPolicy::RoundRobin;
    return true;
  } else if (s == "largest_frac") {
    out = RemainderPolicy::LargestFrac;
    return true;
  } else {
    error_message = "Invalid remainder policy: '" + s +
                    "'. Valid values: " + kValidRemainder;
    return false;
  }
}

bool CalculationOptions::parse_budget(const std::string& s,
                                      BudgetPolicy& out,
                                      std::string& error_message) {
  if (s == "ignore") {
    out = BudgetPolicy::Ignore;
    return true;
  } else if (s == "warn") {
    out = BudgetPolicy::Warn;
    return true;
  } else if (s == "cap") {
    out = BudgetPolicy::Cap;
    return true;
  } else if (s == "spill") {
    out = BudgetPolicy::Spill;
    return true;
  } else {
    error_message = "Invalid budget policy: '" + s +
                    "'. Valid values: " + kValidBudget;
    return false;
  }
}

bool CalculationOptions::parse_granularity(const std::string& s, Tick& out,
                                           std::string& error_message) {
  double hours = 0.0;
  if (!try_parse_double(s, hours) || !std::isfinite(hours) || hours <= 0.0) {
    error_message = "Invalid granularity: '" + s +
                    "'. Expected a positive number of hours";
    return false;
  }
  // Ticks are tenths of an hour, so a quantum must be a whole number of them.
  const double scaled = hours * 10.0;
  const double rounded = std::round(scaled);
  if (std::fabs(scaled - rounded) > 1e-6) {
    error_message = "Invalid granularity: '" + s +
                    "'. Must be a multiple of 0.1 hour";
    return false;
  }
  const long long ticks = static_cast<long long>(rounded);
  if (ticks < 1 || ticks > 24 * 10) {
    error_message = "Invalid granularity: '" + s +
                    "'. Must be between 0.1 and 24.0 hours";
    return false;
  }
  out = static_cast<Tick>(ticks);
  return true;
}

bool CalculationOptions::parse_flag(const std::string& s, bool& out,
                                    std::string& error_message) {
  if (s == "on") {
    out = true;
    return true;
  } else if (s == "off") {
    out = false;
    return true;
  }
  error_message = "Invalid value: '" + s + "'. Valid values: on, off";
  return false;
}

}  // namespace ottr
