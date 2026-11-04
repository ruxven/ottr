#pragma once
#include <string>
#include "model.hpp"  // Needed for World, Aggregation, and Date

namespace ottr {

// Render full multi-day table per SPEC.
std::string render_full_table(const World& world, const Aggregation& agg);

// Render single-day compact table per SPEC (Charge | MM/DD).
std::string render_single_day_table(const World& world, const Aggregation& agg, const Date& day);

// Render the audit trail. Each line shows the allocation and, when rounding
// moved the numbers, a breakdown of the exact share and the deltas applied.
std::string render_audit(const Aggregation& agg);

} // namespace ottr
