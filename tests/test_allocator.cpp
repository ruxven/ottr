#include <gtest/gtest.h>
#include "alloc.hpp"

using namespace ottr;

static Tick get_ticks(const std::vector<AllocationResultEntry>& res, const std::string& id) {
    for (auto &e : res) if (e.charge_id == id) return e.ticks;
    return -1;
}

TEST(Allocator, ExactSplitNoRemainder) {
    AllocationRequest req;
    req.slice_ticks = 30; // 3.0h -> 30 ticks
    req.entries = {
        {"A", 2, 0, 0},
        {"B", 1, 0, 0}
    };
    auto res = allocate_weighted(req);
    ASSERT_EQ(res.size(), 2u);
    EXPECT_EQ(get_ticks(res, "A"), 20);
    EXPECT_EQ(get_ticks(res, "B"), 10);
}

TEST(Allocator, RemainderPriorityThenBudgetThenFracThenLexi) {
    // 1.1h => 11 ticks, weights equal, base 5/5, remainder 1
    AllocationRequest req;
    req.slice_ticks = 11;
    req.entries = {
        {"B", 1, 0, 10}, // remaining budget 10
        {"A", 1, 1, 10}  // higher priority should win remainder
    };
    auto res = allocate_weighted(req);
    EXPECT_EQ(get_ticks(res, "A"), 6);
    EXPECT_EQ(get_ticks(res, "B"), 5);

    // Now equal priority, different budget -> higher remaining budget wins
    req.entries = {
        {"B", 1, 0, 20},
        {"A", 1, 0, 10}
    };
    res = allocate_weighted(req);
    EXPECT_EQ(get_ticks(res, "A"), 5);
    EXPECT_EQ(get_ticks(res, "B"), 6);

    // Equal priority and budget, different fractional parts -> larger frac wins
    // weights 2 and 1 over 11 ticks -> bases 7 and 3 remainder 1 goes to B (frac 2 > 1)
    req.entries = {
        {"A", 2, 0, 10},
        {"B", 1, 0, 10}
    };
    res = allocate_weighted(req);
    EXPECT_EQ(get_ticks(res, "A"), 7);
    EXPECT_EQ(get_ticks(res, "B"), 4);

    // Equal everything including frac: lexi smaller id wins remainder
    req.entries = {
        {"A", 1, 0, 10},
        {"B", 1, 0, 10}
    };
    res = allocate_weighted(req);
    EXPECT_EQ(get_ticks(res, "A"), 6);
    EXPECT_EQ(get_ticks(res, "B"), 5);
}
