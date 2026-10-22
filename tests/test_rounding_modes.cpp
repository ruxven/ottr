#include <gtest/gtest.h>
#include "alloc.hpp"
#include "options.hpp"

using namespace ottr;

class RoundingModesTest : public ::testing::Test {
protected:
    // Helper to create an allocation request with two charges
    AllocationRequest create_request(Tick slice_ticks, int weight1, int weight2) {
        AllocationRequest req;
        req.slice_ticks = slice_ticks;
        
        AllocationEntry e1;
        e1.charge_id = "1234.a";
        e1.weight = weight1;
        e1.priority = 0;
        e1.remaining_budget = 1000;
        req.entries.push_back(e1);
        
        AllocationEntry e2;
        e2.charge_id = "1234.b";
        e2.weight = weight2;
        e2.priority = 0;
        e2.remaining_budget = 1000;
        req.entries.push_back(e2);
        
        return req;
    }

    // Helper to create an equal-weight request across n charges
    AllocationRequest create_equal_request(Tick slice_ticks, int n) {
        AllocationRequest req;
        req.slice_ticks = slice_ticks;
        for (int i = 0; i < n; ++i) {
            AllocationEntry e;
            e.charge_id = "charge_" + std::to_string(i);
            e.weight = 1;
            e.priority = 0;
            e.remaining_budget = 1000;
            req.entries.push_back(e);
        }
        return req;
    }
};

// Test Truncate mode (default, floor division)
TEST_F(RoundingModesTest, TruncateMode_EvenSplit) {
    // 10 ticks, weights 1:1 -> 5:5
    auto req = create_request(10, 1, 1);
    auto result = allocate_weighted(req, RoundingMode::Truncate);
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].charge_id, "1234.a");
    EXPECT_EQ(result[0].ticks, 5);
    EXPECT_EQ(result[1].charge_id, "1234.b");
    EXPECT_EQ(result[1].ticks, 5);
}

TEST_F(RoundingModesTest, TruncateMode_WithRemainder) {
    // 10 ticks, weights 1:2 -> base: 3:6, remainder: 1
    // Remainder goes to higher weight (1234.b)
    auto req = create_request(10, 1, 2);
    auto result = allocate_weighted(req, RoundingMode::Truncate);
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].charge_id, "1234.a");
    EXPECT_EQ(result[0].ticks, 3);
    EXPECT_EQ(result[1].charge_id, "1234.b");
    EXPECT_EQ(result[1].ticks, 7);
}

TEST_F(RoundingModesTest, TruncateMode_OddRemainder) {
    // 7 ticks, weights 1:1 -> base: 3:3, remainder: 1
    auto req = create_request(7, 1, 1);
    auto result = allocate_weighted(req, RoundingMode::Truncate);
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].ticks + result[1].ticks, 7);
}

// Test Round mode (standard rounding)
TEST_F(RoundingModesTest, RoundMode_EvenSplit) {
    // 10 ticks, weights 1:1 -> 5:5
    auto req = create_request(10, 1, 1);
    auto result = allocate_weighted(req, RoundingMode::Round);
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].ticks, 5);
    EXPECT_EQ(result[1].ticks, 5);
}

TEST_F(RoundingModesTest, RoundMode_RoundsUp) {
    // 10 ticks, weights 1:2
    // Ideal: 3.33:6.67 -> rounds to 3:7
    auto req = create_request(10, 1, 2);
    auto result = allocate_weighted(req, RoundingMode::Round);
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].ticks + result[1].ticks, 10);
    // With rounding, we expect 3:7 or similar
}

TEST_F(RoundingModesTest, RoundMode_RoundsDown) {
    // 11 ticks, weights 1:2
    // Ideal: 3.67:7.33 -> rounds to 4:7
    auto req = create_request(11, 1, 2);
    auto result = allocate_weighted(req, RoundingMode::Round);
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].ticks + result[1].ticks, 11);
}

// Test Ceil mode (ceiling division)
TEST_F(RoundingModesTest, CeilMode_EvenSplit) {
    // 10 ticks, weights 1:1 -> 5:5
    auto req = create_request(10, 1, 1);
    auto result = allocate_weighted(req, RoundingMode::Ceil);
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].ticks, 5);
    EXPECT_EQ(result[1].ticks, 5);
}

TEST_F(RoundingModesTest, CeilMode_RoundsUp) {
    // 10 ticks, weights 1:2
    // Ideal: 3.33:6.67 -> ceils to 4:7
    auto req = create_request(10, 1, 2);
    auto result = allocate_weighted(req, RoundingMode::Ceil);
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].ticks + result[1].ticks, 10);
    // With ceiling, we expect higher allocations
}

// Test with three charges
TEST_F(RoundingModesTest, TruncateMode_ThreeCharges) {
    AllocationRequest req;
    req.slice_ticks = 10;
    
    for (int i = 0; i < 3; ++i) {
        AllocationEntry e;
        e.charge_id = "charge_" + std::to_string(i);
        e.weight = 1;
        e.priority = 0;
        e.remaining_budget = 1000;
        req.entries.push_back(e);
    }
    
    auto result = allocate_weighted(req, RoundingMode::Truncate);
    
    EXPECT_EQ(result.size(), 3);
    Tick total = 0;
    for (const auto& r : result) {
        total += r.ticks;
    }
    EXPECT_EQ(total, 10);
}

// Test negative remainder handling (with rounding modes that overshoot)
TEST_F(RoundingModesTest, CeilMode_NegativeRemainder) {
    // 10 ticks, weights 1:1
    // With ceil: 5:5, no negative remainder
    auto req = create_request(10, 1, 1);
    auto result = allocate_weighted(req, RoundingMode::Ceil);
    
    EXPECT_EQ(result[0].ticks + result[1].ticks, 10);
}

// Test that total allocation equals slice_ticks
TEST_F(RoundingModesTest, TotalAllocationPreserved_Truncate) {
    auto req = create_request(17, 3, 5);
    auto result = allocate_weighted(req, RoundingMode::Truncate);
    
    Tick total = 0;
    for (const auto& r : result) {
        total += r.ticks;
    }
    EXPECT_EQ(total, 17);
}

TEST_F(RoundingModesTest, TotalAllocationPreserved_Round) {
    auto req = create_request(17, 3, 5);
    auto result = allocate_weighted(req, RoundingMode::Round);
    
    Tick total = 0;
    for (const auto& r : result) {
        total += r.ticks;
    }
    EXPECT_EQ(total, 17);
}

TEST_F(RoundingModesTest, TotalAllocationPreserved_Ceil) {
    auto req = create_request(17, 3, 5);
    auto result = allocate_weighted(req, RoundingMode::Ceil);
    
    Tick total = 0;
    for (const auto& r : result) {
        total += r.ticks;
    }
    EXPECT_EQ(total, 17);
}

// Remainder must be spread across charges, not dumped on the first winner.
TEST_F(RoundingModesTest, RemainderSpreadsAcrossThreeCharges) {
    auto req = create_equal_request(11, 3);
    auto result = allocate_weighted(req, RoundingMode::Truncate);

    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].ticks, 4);
    EXPECT_EQ(result[1].ticks, 4);
    EXPECT_EQ(result[2].ticks, 3);
}

TEST_F(RoundingModesTest, RemainderSpreadsAcrossFourCharges) {
    auto req = create_equal_request(10, 4);
    auto result = allocate_weighted(req, RoundingMode::Truncate);

    ASSERT_EQ(result.size(), 4);
    EXPECT_EQ(result[0].ticks, 3);
    EXPECT_EQ(result[1].ticks, 3);
    EXPECT_EQ(result[2].ticks, 2);
    EXPECT_EQ(result[3].ticks, 2);
}

// Ceil overshoots when the slice is smaller than the charge count; taking the
// excess back must not drive any charge below zero.
TEST_F(RoundingModesTest, CeilMode_OvershootStaysNonNegative) {
    auto req = create_equal_request(1, 6);
    auto result = allocate_weighted(req, RoundingMode::Ceil);

    ASSERT_EQ(result.size(), 6);
    Tick total = 0;
    for (const auto& r : result) {
        EXPECT_GE(r.ticks, 0) << "charge " << r.charge_id << " went negative";
        total += r.ticks;
    }
    EXPECT_EQ(total, 1);
}

TEST_F(RoundingModesTest, CeilMode_ThreeChargesStaysNonNegative) {
    auto req = create_equal_request(10, 3);
    auto result = allocate_weighted(req, RoundingMode::Ceil);

    ASSERT_EQ(result.size(), 3);
    Tick total = 0;
    for (const auto& r : result) {
        EXPECT_GE(r.ticks, 0);
        total += r.ticks;
    }
    EXPECT_EQ(total, 10);
}

// Higher priority still takes the remainder ahead of lower priority.
TEST_F(RoundingModesTest, PriorityWinsRemainderBeforeSpreading) {
    AllocationRequest req;
    req.slice_ticks = 10;
    for (int i = 0; i < 3; ++i) {
        AllocationEntry e;
        e.charge_id = "charge_" + std::to_string(i);
        e.weight = 1;
        e.priority = (i == 0) ? 5 : 0;
        e.remaining_budget = 1000;
        req.entries.push_back(e);
    }

    auto result = allocate_weighted(req, RoundingMode::Truncate);

    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].ticks, 4);
    EXPECT_EQ(result[1].ticks, 3);
    EXPECT_EQ(result[2].ticks, 3);
}

// Granularity: allocations snap to a multiple of the quantum. A quantum of 5
// ticks is the half hour; 1 tick (the default) is the existing behavior.
TEST_F(RoundingModesTest, Granularity_DefaultMatchesTickAllocation) {
    auto req = create_equal_request(11, 3);
    auto explicit_one = allocate_weighted(req, RoundingMode::Truncate, 1);
    auto defaulted = allocate_weighted(req, RoundingMode::Truncate);

    ASSERT_EQ(explicit_one.size(), defaulted.size());
    for (size_t i = 0; i < defaulted.size(); ++i) {
        EXPECT_EQ(explicit_one[i].ticks, defaulted[i].ticks);
    }
}

TEST_F(RoundingModesTest, Granularity_HalfHourEvenSplit) {
    // 2.0h across 2 charges at half-hour granularity -> 1.0h each
    auto req = create_equal_request(20, 2);
    auto result = allocate_weighted(req, RoundingMode::Truncate, 5);

    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].ticks, 10);
    EXPECT_EQ(result[1].ticks, 10);
}

TEST_F(RoundingModesTest, Granularity_HalfHourUnevenSplit) {
    // 2.0h across 3 charges: ideal 0.67h each, snapped to 0.5h leaves 0.5h over
    auto req = create_equal_request(20, 3);
    auto result = allocate_weighted(req, RoundingMode::Truncate, 5);

    ASSERT_EQ(result.size(), 3);
    Tick total = 0;
    for (const auto& r : result) {
        EXPECT_EQ(r.ticks % 5, 0) << "charge " << r.charge_id << " is not a half-hour multiple";
        total += r.ticks;
    }
    EXPECT_EQ(total, 20);
}

TEST_F(RoundingModesTest, Granularity_AllModesPreserveTotal) {
    const RoundingMode modes[] = {RoundingMode::Truncate, RoundingMode::Round, RoundingMode::Ceil};
    for (RoundingMode mode : modes) {
        auto req = create_equal_request(30, 4);
        auto result = allocate_weighted(req, mode, 5);

        Tick total = 0;
        for (const auto& r : result) {
            EXPECT_GE(r.ticks, 0);
            total += r.ticks;
        }
        EXPECT_EQ(total, 30);
    }
}

// A slice that is not a multiple of the quantum cannot be split into whole
// quanta; the tail must still be charged rather than dropped.
TEST_F(RoundingModesTest, Granularity_SubQuantumTailIsPreserved) {
    auto req = create_equal_request(17, 3);
    auto result = allocate_weighted(req, RoundingMode::Truncate, 5);

    Tick total = 0;
    for (const auto& r : result) {
        total += r.ticks;
    }
    EXPECT_EQ(total, 17);
}

TEST_F(RoundingModesTest, Granularity_CeilOvershootStaysNonNegative) {
    // 0.2h across 6 charges at half-hour granularity: every base ceils to 0.5h,
    // so 3.0h must be clawed back from a 0.2h slice.
    auto req = create_equal_request(2, 6);
    auto result = allocate_weighted(req, RoundingMode::Ceil, 5);

    Tick total = 0;
    for (const auto& r : result) {
        EXPECT_GE(r.ticks, 0) << "charge " << r.charge_id << " went negative";
        total += r.ticks;
    }
    EXPECT_EQ(total, 2);
}

// Remainder policies. One tick is left over after base allocation, and the
// three policies send it to different charges.
static AllocationRequest weighted_request(Tick slice, int w1, int w2, int w3,
                                          int p1, int p2, int p3) {
    AllocationRequest req;
    req.slice_ticks = slice;
    const int weights[3] = {w1, w2, w3};
    const int priorities[3] = {p1, p2, p3};
    for (int i = 0; i < 3; ++i) {
        AllocationEntry e;
        e.charge_id = std::string("charge_") + std::to_string(i);
        e.weight = weights[i];
        e.priority = priorities[i];
        e.remaining_budget = 1000;
        req.entries.push_back(e);
    }
    return req;
}

// Priority outranks the fractional part, so the high-priority charge takes the
// leftover tick even though its fraction is smaller.
TEST_F(RoundingModesTest, RemainderPriorityWinsOverLargestFraction) {
    auto req = weighted_request(10, 1, 2, 3, 0, 5, 0);
    auto result = allocate_weighted(req, RoundingMode::Truncate, 1,
                                    RemainderPolicy::Priority);

    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].ticks, 1);
    EXPECT_EQ(result[1].ticks, 4);
    EXPECT_EQ(result[2].ticks, 5);
}

TEST_F(RoundingModesTest, RemainderLargestFractionWinsOverPriority) {
    auto req = weighted_request(10, 1, 2, 3, 0, 5, 0);
    auto result = allocate_weighted(req, RoundingMode::Truncate, 1,
                                    RemainderPolicy::LargestFrac);

    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].ticks, 2);
    EXPECT_EQ(result[1].ticks, 3);
    EXPECT_EQ(result[2].ticks, 5);
}

TEST_F(RoundingModesTest, RemainderRoundRobinIgnoresPriority) {
    auto req = weighted_request(10, 1, 2, 3, 0, 0, 5);
    auto result = allocate_weighted(req, RoundingMode::Truncate, 1,
                                    RemainderPolicy::RoundRobin);

    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].ticks, 2);
    EXPECT_EQ(result[1].ticks, 3);
    EXPECT_EQ(result[2].ticks, 5);
}

TEST_F(RoundingModesTest, RemainderRoundRobinCyclesEvenly) {
    // Base 3 each and two leftover ticks: first two charges get one each.
    auto req = create_equal_request(11, 3);
    auto result = allocate_weighted(req, RoundingMode::Truncate, 1,
                                    RemainderPolicy::RoundRobin);

    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].ticks, 4);
    EXPECT_EQ(result[1].ticks, 4);
    EXPECT_EQ(result[2].ticks, 3);
}

TEST_F(RoundingModesTest, RemainderPoliciesPreserveTotal) {
    const RemainderPolicy policies[] = {RemainderPolicy::Priority,
                                        RemainderPolicy::RoundRobin,
                                        RemainderPolicy::LargestFrac};
    for (RemainderPolicy policy : policies) {
        auto req = weighted_request(17, 3, 5, 2, 1, 0, 2);
        auto result = allocate_weighted(req, RoundingMode::Truncate, 1, policy);
        Tick total = 0;
        for (const auto& r : result) {
            EXPECT_GE(r.ticks, 0);
            total += r.ticks;
        }
        EXPECT_EQ(total, 17);
    }
}

// Test with large numbers
TEST_F(RoundingModesTest, LargeNumbers_Truncate) {
    auto req = create_request(1000, 1, 1);
    auto result = allocate_weighted(req, RoundingMode::Truncate);
    
    EXPECT_EQ(result[0].ticks, 500);
    EXPECT_EQ(result[1].ticks, 500);
}

TEST_F(RoundingModesTest, LargeNumbers_Round) {
    auto req = create_request(1001, 1, 1);
    auto result = allocate_weighted(req, RoundingMode::Round);
    
    Tick total = result[0].ticks + result[1].ticks;
    EXPECT_EQ(total, 1001);
}
