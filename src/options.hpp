#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <string>

namespace ottr {

enum class RoundingMode {
  Truncate,  // floor division (default)
  Round,     // standard rounding
  Ceil       // ceiling division
};

enum class AggregationMode {
  PerEvent,  // allocate each slice immediately (default)
  PerDay,    // accumulate slices, allocate at end of day
  Off        // explicit no-op (same as PerEvent)
};

struct CalculationOptions {
  RoundingMode rounding = RoundingMode::Truncate;
  AggregationMode aggregation = AggregationMode::PerEvent;

  // Validate options and return error message if invalid
  bool validate(std::string& error_message) const;

  // Parse rounding mode from string
  static bool parse_rounding(const std::string& s, RoundingMode& out,
                             std::string& error_message);

  // Parse aggregation mode from string
  static bool parse_aggregation(const std::string& s, AggregationMode& out,
                                std::string& error_message);
};

}  // namespace ottr

#endif  // OPTIONS_HPP
