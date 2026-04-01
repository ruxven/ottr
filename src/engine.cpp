#include "engine.hpp"
#include "alloc.hpp"
#include "util.hpp"
#include <algorithm>

namespace ottr {

struct RunningTotals {
    // charge_id -> ticks charged so far (within filter window)
    std::unordered_map<std::string, Tick> charged;
};

struct TaskAccumulation {
    std::string task_name;
    Tick accumulated_ticks = 0;
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

    // Helper lambda to process a single slice
    auto process_slice = [&](const Day* day, const Log& start, const Log& end) {
        Tick slice_ticks = end.time_ticks - start.time_ticks;
        if (slice_ticks <= 0) return;
        if (start.kind == LogKind::None) {
            return; // uncharged slice
        }
        if (start.kind == LogKind::Charge) {
            add_allocation(out, day->date, start.ref_id, slice_ticks);
            rt.charged[start.ref_id] += slice_ticks;
            return;
        }
        // Task-based or possibly a direct-charge disguised as task name
        // If ref id is a known charge, treat as direct charge
        auto charge_it = world.charges.find(start.ref_id);
        if (charge_it != world.charges.end()) {
            add_allocation(out, day->date, start.ref_id, slice_ticks);
            rt.charged[start.ref_id] += slice_ticks;
            return;
        }
        auto task_it = world.tasks.find(start.ref_id);
        if (task_it == world.tasks.end()) {
            // Unknown task at runtime; validation should catch. Skip.
            return;
        }
        const Task& task = task_it->second;
        // If no weights -> uncharged
        if (task.weights.empty()) return;

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
        auto result = allocate_weighted(req, opts.calc_options.rounding);
        for (const auto& a : result) {
            if (a.ticks > 0) {
                add_allocation(out, day->date, a.charge_id, a.ticks);
                rt.charged[a.charge_id] += a.ticks;
            }
        }
    };

    // Process based on aggregation mode
    if (opts.calc_options.aggregation == AggregationMode::PerEvent || 
        opts.calc_options.aggregation == AggregationMode::Off) {
        // Per-event: allocate each slice immediately
        for (const Day* day : included_days) {
            const auto& logs = day->logs;
            if (logs.size() < 2) continue; // validation should have caught
            for (size_t i = 0; i + 1 < logs.size(); ++i) {
                process_slice(day, logs[i], logs[i+1]);
            }
        }
    } else if (opts.calc_options.aggregation == AggregationMode::PerDay) {
        // Per-day: accumulate all task slices per day, then allocate at day end
        for (const Day* day : included_days) {
            const auto& logs = day->logs;
            if (logs.size() < 2) continue;

            // Accumulate task times for this day
            std::unordered_map<std::string, Tick> task_accumulation;
            std::vector<std::pair<size_t, Tick>> charge_slices; // (log_index, slice_ticks)

            for (size_t i = 0; i + 1 < logs.size(); ++i) {
                const Log& start = logs[i];
                const Log& end = logs[i+1];
                Tick slice_ticks = end.time_ticks - start.time_ticks;
                if (slice_ticks <= 0) continue;
                if (start.kind == LogKind::None) {
                    continue; // uncharged slice
                }
                if (start.kind == LogKind::Charge) {
                    // Direct charge: allocate immediately
                    charge_slices.push_back({i, slice_ticks});
                    continue;
                }

                // Check if it's a known charge
                auto charge_it = world.charges.find(start.ref_id);
                if (charge_it != world.charges.end()) {
                    charge_slices.push_back({i, slice_ticks});
                    continue;
                }

                // It's a task: accumulate
                auto task_it = world.tasks.find(start.ref_id);
                if (task_it == world.tasks.end()) {
                    continue;
                }
                const Task& task = task_it->second;
                if (task.weights.empty()) continue;

                task_accumulation[start.ref_id] += slice_ticks;
            }

            // First, process direct charges
            for (const auto& [log_idx, slice_ticks] : charge_slices) {
                const Log& start = logs[log_idx];
                const Log& end = logs[log_idx + 1];
                process_slice(day, start, end);
            }

            // Then, allocate accumulated task times
            for (const auto& [task_name, total_ticks] : task_accumulation) {
                auto task_it = world.tasks.find(task_name);
                if (task_it == world.tasks.end()) continue;
                const Task& task = task_it->second;
                if (task.weights.empty()) continue;

                // Build allocation request for accumulated time
                AllocationRequest req;
                req.slice_ticks = total_ticks;
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
                        continue;
                    }
                    req.entries.push_back(std::move(e));
                }
                auto result = allocate_weighted(req, opts.calc_options.rounding);
                for (const auto& a : result) {
                    if (a.ticks > 0) {
                        add_allocation(out, day->date, a.charge_id, a.ticks);
                        rt.charged[a.charge_id] += a.ticks;
                    }
                }
            }
        }
    }

    return true;
}

} // namespace ottr
