#include "alloc.hpp"
#include <algorithm>
#include <numeric>

namespace ottr {

namespace {

struct WorkItem {
    size_t idx = 0;        // index into req.entries
    long long base = 0;    // allocation before remainder distribution
    long long frac = 0;    // distance past the quantum the ideal share fell
    long long adjusts = 0; // quanta added or removed during remainder distribution
};

// Round ideal_numer/denom to a multiple of quantum, per the rounding mode. The
// division is scaled by the quantum so every mode lands on a quantum boundary
// rather than rounding to a tick and then snapping.
long long quantize(long long ideal_numer, long long denom, RoundingMode rounding,
                   long long quantum) {
    const long long scaled = denom * quantum;
    long long units = 0;
    switch (rounding) {
        case RoundingMode::Round:
            units = (ideal_numer + scaled / 2) / scaled;
            break;
        case RoundingMode::Ceil:
            units = (ideal_numer + scaled - 1) / scaled;
            break;
        case RoundingMode::Truncate:
        default:
            units = ideal_numer / scaled;
            break;
    }
    return units * quantum;
}

// Rank two candidates for receiving (descending) or surrendering (ascending) a
// quantum. Returns true when a outranks b. The policy only changes which
// criteria come first; the trailing criteria keep the order total so the
// distribution always terminates and stays deterministic.
bool outranks(const AllocationEntry& ea, const WorkItem& wa,
              const AllocationEntry& eb, const WorkItem& wb,
              bool descending, RemainderPolicy policy) {
    auto prefer = [descending](long long a, long long b) {
        return descending ? (a > b) : (a < b);
    };

    if (policy == RemainderPolicy::LargestFrac) {
        if (wa.frac != wb.frac) return prefer(wa.frac, wb.frac);
        if (ea.priority != eb.priority) return prefer(ea.priority, eb.priority);
    } else {
        if (ea.priority != eb.priority) return prefer(ea.priority, eb.priority);
    }

    if (ea.remaining_budget != eb.remaining_budget) {
        return prefer(ea.remaining_budget, eb.remaining_budget);
    }
    if (policy != RemainderPolicy::LargestFrac && wa.frac != wb.frac) {
        return prefer(wa.frac, wb.frac);
    }
    // Without this the comparison is identical on every iteration and a single
    // charge absorbs the whole remainder.
    if (wa.adjusts != wb.adjusts) {
        return wa.adjusts < wb.adjusts;
    }
    return ea.charge_id < eb.charge_id;
}

long long total_weight(const AllocationRequest& req) {
    long long denom = 0;
    for (const auto& e : req.entries) {
        if (e.weight > 0) denom += e.weight;
    }
    return denom;
}

std::vector<WorkItem> build_work(const AllocationRequest& req, long long denom,
                                 RoundingMode rounding, long long quantum) {
    std::vector<WorkItem> work;
    work.reserve(req.entries.size());
    for (size_t i = 0; i < req.entries.size(); ++i) {
        const AllocationEntry& e = req.entries[i];
        if (e.weight <= 0) continue;
        const long long ideal_numer =
            static_cast<long long>(req.slice_ticks) * static_cast<long long>(e.weight);
        const long long base = quantize(ideal_numer, denom, rounding, quantum);
        work.push_back(WorkItem{i, base, ideal_numer % (denom * quantum), 0});
    }
    return work;
}

// Hand out or claw back whole quanta until the allocation sums to slice_ticks.
// A slice that is not a multiple of the quantum leaves a sub-quantum tail; it
// goes to the next eligible charge so the total is always preserved exactly.
void distribute_ranked(const AllocationRequest& req, std::vector<WorkItem>& work,
                       std::vector<AllocationResultEntry>& out, long long quantum,
                       long long remainder, RemainderPolicy policy) {
    // Negative remainder means the rounding mode overshot the slice. Only
    // charges holding at least the amount being taken are eligible, so no
    // allocation can go negative.
    auto pick = [&](bool descending, long long needed) -> WorkItem* {
        WorkItem* best = nullptr;
        for (WorkItem& w : work) {
            if (out[w.idx].ticks < needed) continue;
            if (best == nullptr ||
                outranks(req.entries[w.idx], w, req.entries[best->idx], *best,
                         descending, policy)) {
                best = &w;
            }
        }
        return best;
    };

    while (remainder != 0) {
        const bool giving = remainder > 0;
        const long long magnitude = giving ? remainder : -remainder;
        const long long step = std::min(quantum, magnitude);
        WorkItem* w = pick(giving, giving ? 0 : step);
        if (w == nullptr) break;
        out[w->idx].ticks += giving ? step : -step;
        w->adjusts += 1;
        remainder += giving ? -step : step;
    }
}

// Give each eligible charge one quantum in turn, ignoring priority and budget.
// Subtracting follows the same cycle, skipping charges that cannot cover the
// step.
void distribute_round_robin(std::vector<WorkItem>& work,
                            std::vector<AllocationResultEntry>& out,
                            long long quantum, long long remainder) {
    size_t cursor = 0;
    while (remainder != 0 && !work.empty()) {
        const bool giving = remainder > 0;
        const long long magnitude = giving ? remainder : -remainder;
        const long long step = std::min(quantum, magnitude);

        bool advanced = false;
        for (size_t k = 0; k < work.size(); ++k) {
            const size_t pos = (cursor + k) % work.size();
            WorkItem& w = work[pos];
            if (!giving && out[w.idx].ticks < step) continue;
            out[w.idx].ticks += giving ? step : -step;
            w.adjusts += 1;
            remainder += giving ? -step : step;
            cursor = (pos + 1) % work.size();
            advanced = true;
            break;
        }
        if (!advanced) break;
    }
}

} // namespace

std::vector<AllocationResultEntry> allocate_weighted(const AllocationRequest& req,
                                                      RoundingMode rounding,
                                                      Tick granularity,
                                                      RemainderPolicy remainder) {
    std::vector<AllocationResultEntry> out;
    if (req.slice_ticks <= 0 || req.entries.empty()) return out;

    const long long denom = total_weight(req);
    if (denom <= 0) return out; // uncharged per spec

    const long long quantum = granularity > 0 ? static_cast<long long>(granularity) : 1;

    out.resize(req.entries.size());
    for (size_t i = 0; i < req.entries.size(); ++i) {
        out[i].charge_id = req.entries[i].charge_id;
        out[i].ticks = 0;
    }

    std::vector<WorkItem> work = build_work(req, denom, rounding, quantum);
    for (const WorkItem& w : work) {
        out[w.idx].ticks += static_cast<Tick>(w.base);
    }

    long long base_sum = 0;
    for (const WorkItem& w : work) base_sum += w.base;
    const long long leftover = static_cast<long long>(req.slice_ticks) - base_sum;

    if (remainder == RemainderPolicy::RoundRobin) {
        distribute_round_robin(work, out, quantum, leftover);
    } else {
        distribute_ranked(req, work, out, quantum, leftover, remainder);
    }

    return out;
}

} // namespace ottr
