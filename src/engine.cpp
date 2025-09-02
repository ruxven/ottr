#include "engine.hpp"
#include "alloc.hpp"
#include "util.hpp"
#include <algorithm>

namespace ottr {

struct RunningTotals {
    // charge_id -> ticks charged so far (within filter window)
    std::unordered_map<std::string, Tick> charged;
};

static void add_allocation(Aggregation& agg, const Date& d, const std::string& cid, Tick ticks) {
    if (ticks <= 0) return;
    agg.by_charge_by_day[cid][d.raw] += ticks;
    agg.totals_by_charge[cid] += ticks;
    agg.totals_by_day[d.raw] += ticks;
}

bool process_world(const World& world, const EngineOptions& opts, Aggregation& out, std::string& error_message) {
    out = Aggregation{};

    // Build list of included dates in order
    std::vector<const Day*> included_days;
    included_days.reserve(world.days.size());
    for (const auto& day : world.days) {
        if (opts.filter.includes(day.date)) {
            included_days.push_back(&day);
        }
    }
    std::sort(included_days.begin(), included_days.end(), [](const Day* a, const Day* b){ return a->date.ord < b->date.ord; });
    out.dates_in_order.reserve(included_days.size());
    for (auto* d : included_days) out.dates_in_order.push_back(d->date);

    RunningTotals rt;

    for (const Day* day : included_days) {
        const auto& logs = day->logs;
        if (logs.size() < 2) continue; // validation should have caught
        for (size_t i = 0; i + 1 < logs.size(); ++i) {
            const Log& start = logs[i];
            const Log& end = logs[i+1];
            Tick slice_ticks = end.time_ticks - start.time_ticks;
            if (slice_ticks <= 0) continue;
            if (start.kind == LogKind::None) {
                continue; // uncharged slice
            }
            if (start.kind == LogKind::Charge) {
                add_allocation(out, day->date, start.ref_id, slice_ticks);
                rt.charged[start.ref_id] += slice_ticks;
                continue;
            }
            // Task-based or possibly a direct-charge disguised as task name
            // If ref id is a known charge, treat as direct charge
            auto charge_it = world.charges.find(start.ref_id);
            if (charge_it != world.charges.end()) {
                add_allocation(out, day->date, start.ref_id, slice_ticks);
                rt.charged[start.ref_id] += slice_ticks;
                continue;
            }
            auto task_it = world.tasks.find(start.ref_id);
            if (task_it == world.tasks.end()) {
                // Unknown task at runtime; validation should catch. Skip.
                continue;
            }
            const Task& task = task_it->second;
            // If no weights -> uncharged
            if (task.weights.empty()) continue;

            // Build allocation request
            AllocationRequest req;
            req.slice_ticks = slice_ticks;
            req.entries.reserve(task.weights.size());
            for (const auto& w : task.weights) {
                AllocationEntry e;
                e.charge_id = w.first;
                e.weight = w.second;
                auto cit = world.charges.find(e.charge_id);
                if (cit != world.charges.end()) {
                    e.priority = cit->second.priority;
                    e.remaining_budget = cit->second.budget_ticks - (rt.charged[e.charge_id]);
                } else {
                    // Unknown charge in weights -> skip; validator should catch
                    continue;
                }
                // Record fractional info will be computed by allocator
                req.entries.push_back(std::move(e));
            }
            auto result = allocate_weighted(req);
            for (const auto& a : result) {
                if (a.ticks > 0) {
                    add_allocation(out, day->date, a.charge_id, a.ticks);
                    rt.charged[a.charge_id] += a.ticks;
                }
            }
        }
    }

    return true;
}

} // namespace ottr
