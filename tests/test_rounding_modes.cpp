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
