#pragma once
#include <string>
#include <vector>
#include "model.hpp"
#include "options.hpp"

namespace ottr {

struct AllocationEntry {
    std::string charge_id;
    int weight = 0;
    int priority = 0;
    long long remaining_budget = 0; // can be negative
};

struct AllocationResultEntry {
    std::string charge_id;
    Tick ticks = 0;
};

struct AllocationRequest {
    Tick slice_ticks = 0;
    std::vector<AllocationEntry> entries;
};

// Weighted allocation per DESIGN: integer division + remainder distribution.
// Applies rounding mode to base allocations and distributes remainder.
std::vector<AllocationResultEntry> allocate_weighted(const AllocationRequest& req,
                                                      RoundingMode rounding = RoundingMode::Truncate);

} // namespace ottr
