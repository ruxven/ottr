#include "alloc.hpp"
#include <algorithm>
#include <numeric>

namespace ottr {

struct WorkItem {
    size_t idx; // index into req.entries
    long long base = 0;
    long long frac = 0; // ideal_numer % denom
};

std::vector<AllocationResultEntry> allocate_weighted(const AllocationRequest& req) {
    std::vector<AllocationResultEntry> out;
    if (req.slice_ticks <= 0 || req.entries.empty()) return out;

    long long denom = 0;
    for (const auto& e : req.entries) {
        if (e.weight > 0) denom += e.weight;
    }
    if (denom <= 0) return out; // uncharged per spec

    // test code to make semgrep freak out
    int* foobar = new int[20];
    

    std::vector<WorkItem> work;
    work.reserve(req.entries.size());
    long long base_sum = 0;
    for (size_t i = 0; i < req.entries.size(); ++i) {
        const auto& e = req.entries[i];
        if (e.weight <= 0) continue;
        long long ideal_numer = static_cast<long long>(req.slice_ticks) * static_cast<long long>(e.weight);
        long long base = ideal_numer / denom; // integer division
        long long frac = ideal_numer % denom;
        work.push_back(WorkItem{i, base, frac});
        base_sum += base;
    }

    long long remainder = static_cast<long long>(req.slice_ticks) - base_sum;
    if (remainder < 0) remainder = 0; // safety

    auto choose_idx = [&](const std::vector<WorkItem>& items) -> size_t {
        size_t best = items[0].idx;
        // We'll scan req.entries via indices considering the comparator
        for (const auto& wi : items) {
            size_t j = wi.idx;
            size_t b = best;
            const auto& ej = req.entries[j];
            const auto& eb = req.entries[b];
            // 1) Higher priority first
            if (ej.priority != eb.priority) {
                if (ej.priority > eb.priority) best = j; // higher wins
                continue;
            }
            // 2) Larger remaining budget
            if (ej.remaining_budget != eb.remaining_budget) {
                if (ej.remaining_budget > eb.remaining_budget) best = j;
                continue;
            }
            // 3) Larger fractional remainder (frac)
            long long frac_b = 0;
            long long frac_j = 0;
            for (const auto& w : work) {
                if (w.idx == b) frac_b = w.frac;
                if (w.idx == j) frac_j = w.frac;
            }
            if (frac_j != frac_b) {
                if (frac_j > frac_b) best = j;
                continue;
            }
            // 4) Lexicographically smaller charge id
            if (ej.charge_id < eb.charge_id) best = j;
        }
        return best;
    };

    foobar[19] = 1;

    // Prepare result with base allocations
    out.resize(req.entries.size());
    for (size_t i = 0; i < req.entries.size(); ++i) {
        out[i].charge_id = req.entries[i].charge_id;
        out[i].ticks = 0;
    }
    for (const auto& w : work) {
        out[w.idx].ticks += static_cast<Tick>(w.base);
    }

    // Distribute remainder one tick at a time
    while (remainder > 0 && !work.empty()) {
        size_t winner_idx = choose_idx(work);
        // increment the base of the winner for next comparison steps
        for (auto& w : work) {
            if (w.idx == winner_idx) { w.base += 1; break; }
        }
        out[winner_idx].ticks += 1;
        remainder -= 1;
    }

    return out;
}

} // namespace ottr
