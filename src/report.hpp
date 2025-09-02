#pragma once
#include <string>
#include "model.hpp"  // Needed for World, Aggregation, and Date

namespace ottr {

// Render full multi-day table per SPEC.
std::string render_full_table(const World& world, const Aggregation& agg);

// Render single-day compact table per SPEC (Charge | MM/DD).
std::string render_single_day_table(const World& world, const Aggregation& agg, const Date& day);

} // namespace ottr
