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
    // Rounding detail: the exact proportional share of the slice is
    // ideal_numer/ideal_denom ticks; rounded_ticks is the base allocation
    // after the rounding mode; adjust_ticks is the net number of quanta the
    // remainder policy added (+) or removed (-).
    long long ideal_numer = 0;
    long long ideal_denom = 1;
    Tick rounded_ticks = 0;
    Tick adjust_ticks = 0;
};

struct AllocationRequest {
    Tick slice_ticks = 0;
    std::vector<AllocationEntry> entries;
};

// Weighted allocation per DESIGN: integer division + remainder distribution.
// Applies rounding mode to base allocations and distributes remainder.
// granularity is the charge quantum in ticks; 1 charges to the tenth of an hour,
// 5 to the half hour. Allocations are multiples of it except for a sub-quantum
// tail when slice_ticks is not itself a multiple.
// remainder selects how leftover quanta are handed out.
std::vector<AllocationResultEntry> allocate_weighted(
    const AllocationRequest& req,
    RoundingMode rounding = RoundingMode::Truncate,
    Tick granularity = 1,
    RemainderPolicy remainder = RemainderPolicy::Priority);

} // namespace ottr
