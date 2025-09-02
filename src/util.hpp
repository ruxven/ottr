#pragma once
#include <string>
#include <cmath>
#include "model.hpp"

namespace ottr {

// Parse MM/DD into Date, return false if invalid
bool parse_mmdd(const std::string& s, Date& out);

// Convert decimal hours to ticks (tenths), with floor rounding.
inline Tick hours_to_ticks(double h) {
    // Add a tiny epsilon to avoid edge cases due to binary float representation
    const double eps = 1e-9;
    long long t = static_cast<long long>(std::floor(h * 10.0 + eps));
    return static_cast<Tick>(t);
}

// Try-parse helpers
bool try_parse_double(const std::string& s, double& out);
bool try_parse_ll(const std::string& s, long long& out);

} // namespace ottr
