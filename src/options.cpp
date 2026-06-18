#include "options.hpp"

namespace ottr {

bool CalculationOptions::validate([[maybe_unused]] std::string& error_message) const {
  // All enum values are valid by construction, so validation always passes
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
                    "'. Valid values: truncate, round, ceil";
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
  } else if (s == "off") {
    out = AggregationMode::Off;
    return true;
  } else {
    error_message = "Invalid aggregation mode: '" + s +
                    "'. Valid values: per_event, per_day, off";
    return false;
  }
}

}  // namespace ottr
