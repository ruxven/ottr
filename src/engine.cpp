#include "engine.hpp"
#include "alloc.hpp"
#include "util.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace ottr {

namespace {

struct RunningTotals {
    // charge_id -> ticks charged so far (within filter window)
    std::unordered_map<std::string, Tick> charged;
};

enum class SliceKind {
    Uncharged, // no ref, unknown ref, or a task carrying no weights
    Direct,    // ref names a charge number; the whole slice goes to it
    Task       // ref names a task with weights; split by the allocator
};

struct Slice {
    SliceKind kind = SliceKind::Uncharged;
    std::string ref_id;
    Tick ticks = 0;
};

void add_allocation(Aggregation& agg, const Date& d, const std::string& cid, Tick ticks) {
    if (ticks <= 0) return;
    agg.by_charge_by_day[cid][d.raw] += ticks;
    agg.totals_by_charge[cid] += ticks;
    agg.totals_by_day[d.raw] += ticks;
}

void apply_charge(Aggregation& out, RunningTotals& rt, const Date& date,
                  const std::string& note, const std::string& source,
                  const std::string& charge_id, Tick ticks, bool audit) {
    if (ticks <= 0) return;
    add_allocation(out, date, charge_id, ticks);
    rt.charged[charge_id] += ticks;
    if (audit) {
        out.audit.push_back(AuditEntry{date, note, source, charge_id, ticks});
    }
}

Tick remaining_budget(const World& world, const RunningTotals& rt,
                      const std::string& charge_id) {
    auto cit = world.charges.find(charge_id);
    if (cit == world.charges.end()) return 0;
    auto it = rt.charged.find(charge_id);
    Tick charged = (it == rt.charged.end()) ? 0 : it->second;
    return cit->second.budget_ticks - charged;
}

std::string hours_text(Tick ticks) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision(1) << (static_cast<double>(ticks) / 10.0);
    return oss.str();
}

SliceKind classify_ref(const World& world, const Log& start) {
    if (start.kind == LogKind::None) return SliceKind::Uncharged;
    if (start.kind == LogKind::Charge) return SliceKind::Direct;
    // A task-kind log may still name a charge number directly.
    if (world.charges.count(start.ref_id)) return SliceKind::Direct;
    auto task_it = world.tasks.find(start.ref_id);
    if (task_it == world.tasks.end()) return SliceKind::Uncharged;
    if (task_it->second.weights.empty()) return SliceKind::Uncharged;
    return SliceKind::Task;
}

// Each non-final log opens a slice that ends at the next log.
std::vector<Slice> day_slices(const World& world, const Day& day) {
    std::vector<Slice> slices;
    const auto& logs = day.logs;
    if (logs.size() < 2) return slices; // validation should have caught
    slices.reserve(logs.size() - 1);
    for (size_t i = 0; i + 1 < logs.size(); ++i) {
        const Log& start = logs[i];
        Tick ticks = logs[i + 1].time_ticks - start.time_ticks;
        if (ticks <= 0) continue;
        SliceKind kind = classify_ref(world, start);
        if (kind == SliceKind::Uncharged) continue;
        slices.push_back(Slice{kind, start.ref_id, ticks});
    }
    return slices;
}

// Merge task slices within a day. Direct charges keep their own slices and are
// allocated first. Tasks are ordered by first appearance so the allocation
// sequence follows the file, per SPEC.txt per_day.
std::vector<Slice> group_day_slices(const std::vector<Slice>& slices) {
    std::vector<Slice> grouped;
    std::vector<std::string> task_order;
    std::unordered_map<std::string, Tick> task_totals;
    for (const Slice& s : slices) {
        if (s.kind == SliceKind::Direct) {
            grouped.push_back(s);
            continue;
        }
        if (task_totals.find(s.ref_id) == task_totals.end()) {
            task_order.push_back(s.ref_id);
        }
        task_totals[s.ref_id] += s.ticks;
    }
    grouped.reserve(grouped.size() + task_order.size());
    for (const std::string& name : task_order) {
        grouped.push_back(Slice{SliceKind::Task, name, task_totals[name]});
    }
    return grouped;
}

AllocationRequest build_request(const World& world, const RunningTotals& rt,
                                const Task& task, Tick ticks) {
    AllocationRequest req;
    req.slice_ticks = ticks;
    req.entries.reserve(task.weights.size());
    for (const auto& w : task.weights) {
        auto cit = world.charges.find(w.first);
        if (cit == world.charges.end()) continue; // validator should catch
        AllocationEntry e;
        e.charge_id = w.first;
        e.weight = w.second;
        e.priority = cit->second.priority;
        e.remaining_budget = remaining_budget(world, rt, e.charge_id);
        req.entries.push_back(std::move(e));
    }
    // task.weights is unordered; sort so the request is reproducible.
    std::sort(req.entries.begin(), req.entries.end(),
              [](const AllocationEntry& a, const AllocationEntry& b) {
                  return a.charge_id < b.charge_id;
              });
    return req;
}

void warn_overrun(const Task& task, const std::string& charge_id, Tick over,
                  const Date& date, std::vector<std::string>* warnings) {
    if (warnings == nullptr) return;
    std::ostringstream oss;
    oss << "warning: charge '" << charge_id << "' over budget by "
        << hours_text(over) << " hours on " << date.raw
        << " (task '" << task.name << "')";
    warnings->push_back(oss.str());
}

// Ignore and Warn allocate exactly as before; Warn additionally reports any
// charge that received more than its remaining budget.
std::vector<AllocationResultEntry> allocate_unbounded(const World& world,
                                                      const EngineOptions& opts,
                                                      const Task& task, Tick ticks,
                                                      const RunningTotals& rt) {
    const CalculationOptions& co = opts.calc_options;
    AllocationRequest req = build_request(world, rt, task, ticks);
    return allocate_weighted(req, co.rounding, co.granularity, co.remainder);
}

// Cap and Spill repeatedly allocate what is left across charges that still have
// budget. Each pass can only reduce the remaining amount, so this terminates;
// if no charge can take anything, the rest is dropped (Cap) or spilled.
Tick allocate_budgeted(const World& world, const EngineOptions& opts, const Date& date,
                       const Task& task, Tick ticks, RunningTotals& rt,
                       Aggregation& out) {
    const CalculationOptions& co = opts.calc_options;
    Tick remaining = ticks;
    while (remaining > 0) {
        AllocationRequest req = build_request(world, rt, task, remaining);
        req.entries.erase(
            std::remove_if(req.entries.begin(), req.entries.end(),
                           [](const AllocationEntry& e) { return e.remaining_budget <= 0; }),
            req.entries.end());
        if (req.entries.empty()) break;

        auto result = allocate_weighted(req, co.rounding, co.granularity, co.remainder);
        Tick applied = 0;
        for (const auto& a : result) {
            if (a.ticks <= 0) continue;
            Tick cap = std::max<Tick>(0, remaining_budget(world, rt, a.charge_id));
            Tick take = std::min<Tick>(a.ticks, cap);
            if (take <= 0) continue;
            apply_charge(out, rt, date, "task", task.name, a.charge_id, take, co.audit);
            applied += take;
        }
        if (applied <= 0) break;
        remaining -= applied;
    }
    return remaining;
}

void allocate_task(const World& world, const EngineOptions& opts, const Date& date,
                   const Task& task, Tick ticks, RunningTotals& rt, Aggregation& out,
                   std::vector<std::string>* warnings) {
    const CalculationOptions& co = opts.calc_options;

    if (co.budget == BudgetPolicy::Ignore || co.budget == BudgetPolicy::Warn) {
        auto result = allocate_unbounded(world, opts, task, ticks, rt);
        for (const auto& a : result) {
            if (a.ticks <= 0) continue;
            if (co.budget == BudgetPolicy::Warn) {
                Tick cap = remaining_budget(world, rt, a.charge_id);
                if (a.ticks > cap) warn_overrun(task, a.charge_id, a.ticks - cap, date, warnings);
            }
            apply_charge(out, rt, date, "task", task.name, a.charge_id, a.ticks, co.audit);
        }
        return;
    }

    Tick leftover = allocate_budgeted(world, opts, date, task, ticks, rt, out);
    if (co.budget == BudgetPolicy::Spill && leftover > 0) {
        apply_charge(out, rt, date, "spill", task.name, co.overflow_charge, leftover, co.audit);
    }
}

void allocate_direct(const World& world, const EngineOptions& opts, const Date& date,
                     const Slice& slice, RunningTotals& rt, Aggregation& out,
                     std::vector<std::string>* warnings) {
    const CalculationOptions& co = opts.calc_options;
    Tick capacity = remaining_budget(world, rt, slice.ref_id);

    if (co.budget == BudgetPolicy::Warn && slice.ticks > capacity) {
        if (warnings != nullptr) {
            std::ostringstream oss;
            oss << "warning: charge '" << slice.ref_id << "' over budget by "
                << hours_text(slice.ticks - capacity) << " hours on " << date.raw
                << " (direct charge)";
            warnings->push_back(oss.str());
        }
    }

    Tick take = slice.ticks;
    if (co.budget == BudgetPolicy::Cap || co.budget == BudgetPolicy::Spill) {
        take = std::min<Tick>(slice.ticks, std::max<Tick>(0, capacity));
    }
    apply_charge(out, rt, date, "direct", slice.ref_id, slice.ref_id, take, co.audit);

    Tick leftover = slice.ticks - take;
    if (co.budget == BudgetPolicy::Spill && leftover > 0) {
        apply_charge(out, rt, date, "spill", slice.ref_id, co.overflow_charge,
                     leftover, co.audit);
    }
}

void allocate_slice(const World& world, const EngineOptions& opts, const Date& date,
                    const Slice& slice, RunningTotals& rt, Aggregation& out,
                    std::vector<std::string>* warnings) {
    if (slice.kind == SliceKind::Direct) {
        allocate_direct(world, opts, date, slice, rt, out, warnings);
        return;
    }
    auto task_it = world.tasks.find(slice.ref_id);
    if (task_it == world.tasks.end()) return;
    allocate_task(world, opts, date, task_it->second, slice.ticks, rt, out, warnings);
}

std::vector<const Day*> included_days(const World& world, const DateFilter& filter) {
    std::vector<const Day*> days;
    days.reserve(world.days.size());
    for (const auto& day : world.days) {
        if (filter.includes(day.date)) days.push_back(&day);
    }
    std::sort(days.begin(), days.end(),
              [](const Day* a, const Day* b) { return a->date.ord < b->date.ord; });
    return days;
}

int month_of(const Date& date) { return date.ord / 100; }

// PerMonth: accumulate task time across every included day of a month and
// allocate it at the month's last included day. Direct charges stay on their own
// day and are allocated as encountered.
void process_months(const World& world, const EngineOptions& opts,
                    const std::vector<const Day*>& days, RunningTotals& rt,
                    Aggregation& out, std::vector<std::string>* warnings) {
    size_t i = 0;
    while (i < days.size()) {
        const int month = month_of(days[i]->date);
        std::vector<std::string> task_order;
        std::unordered_map<std::string, Tick> task_totals;
        Date last_date = days[i]->date;

        for (; i < days.size() && month_of(days[i]->date) == month; ++i) {
            last_date = days[i]->date;
            for (const Slice& s : day_slices(world, *days[i])) {
                if (s.kind == SliceKind::Direct) {
                    allocate_slice(world, opts, days[i]->date, s, rt, out, warnings);
                    continue;
                }
                if (task_totals.find(s.ref_id) == task_totals.end()) {
                    task_order.push_back(s.ref_id);
                }
                task_totals[s.ref_id] += s.ticks;
            }
        }

        for (const std::string& name : task_order) {
            auto task_it = world.tasks.find(name);
            if (task_it == world.tasks.end()) continue;
            allocate_task(world, opts, last_date, task_it->second,
                          task_totals[name], rt, out, warnings);
        }
    }
}

} // namespace

bool process_world(const World& world, const EngineOptions& opts, Aggregation& out,
                   [[maybe_unused]] std::string& error_message,
                   std::vector<std::string>* warnings) {
    out = Aggregation{};

    std::vector<const Day*> days = included_days(world, opts.filter);
    out.dates_in_order.reserve(days.size());
    for (const Day* d : days) out.dates_in_order.push_back(d->date);

    RunningTotals rt;
    if (opts.calc_options.aggregation == AggregationMode::PerMonth) {
        process_months(world, opts, days, rt, out, warnings);
        return true;
    }

    const bool merge_day = opts.calc_options.aggregation == AggregationMode::PerDay;
    for (const Day* day : days) {
        std::vector<Slice> slices = day_slices(world, *day);
        if (merge_day) slices = group_day_slices(slices);
        for (const Slice& slice : slices) {
            allocate_slice(world, opts, day->date, slice, rt, out, warnings);
        }
    }

    return true;
}

} // namespace ottr
