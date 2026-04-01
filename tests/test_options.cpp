#include <gtest/gtest.h>
#include "options.hpp"

using namespace ottr;

// Test RoundingMode parsing
TEST(OptionsTest, ParseRoundingTruncate) {
    RoundingMode mode;
    std::string error;
    EXPECT_TRUE(CalculationOptions::parse_rounding("truncate", mode, error));
    EXPECT_EQ(mode, RoundingMode::Truncate);
}

TEST(OptionsTest, ParseRoundingRound) {
    RoundingMode mode;
    std::string error;
    EXPECT_TRUE(CalculationOptions::parse_rounding("round", mode, error));
    EXPECT_EQ(mode, RoundingMode::Round);
}

TEST(OptionsTest, ParseRoundingCeil) {
    RoundingMode mode;
    std::string error;
    EXPECT_TRUE(CalculationOptions::parse_rounding("ceil", mode, error));
    EXPECT_EQ(mode, RoundingMode::Ceil);
}

TEST(OptionsTest, ParseRoundingInvalid) {
    RoundingMode mode;
    std::string error;
    EXPECT_FALSE(CalculationOptions::parse_rounding("invalid", mode, error));
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find("Invalid rounding mode") != std::string::npos);
}

// Test AggregationMode parsing
TEST(OptionsTest, ParseAggregationPerEvent) {
    AggregationMode mode;
    std::string error;
    EXPECT_TRUE(CalculationOptions::parse_aggregation("per_event", mode, error));
    EXPECT_EQ(mode, AggregationMode::PerEvent);
}

TEST(OptionsTest, ParseAggregationPerDay) {
    AggregationMode mode;
    std::string error;
    EXPECT_TRUE(CalculationOptions::parse_aggregation("per_day", mode, error));
    EXPECT_EQ(mode, AggregationMode::PerDay);
}

TEST(OptionsTest, ParseAggregationOff) {
    AggregationMode mode;
    std::string error;
    EXPECT_TRUE(CalculationOptions::parse_aggregation("off", mode, error));
    EXPECT_EQ(mode, AggregationMode::Off);
}

TEST(OptionsTest, ParseAggregationInvalid) {
    AggregationMode mode;
    std::string error;
    EXPECT_FALSE(CalculationOptions::parse_aggregation("invalid", mode, error));
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find("Invalid aggregation mode") != std::string::npos);
}

// Test CalculationOptions defaults
TEST(OptionsTest, DefaultOptions) {
    CalculationOptions opts;
    EXPECT_EQ(opts.rounding, RoundingMode::Truncate);
    EXPECT_EQ(opts.aggregation, AggregationMode::PerEvent);
}

// Test CalculationOptions validation
TEST(OptionsTest, ValidateOptions) {
    CalculationOptions opts;
    std::string error;
    EXPECT_TRUE(opts.validate(error));
}

// Test setting options
TEST(OptionsTest, SetRoundingMode) {
    CalculationOptions opts;
    opts.rounding = RoundingMode::Round;
    EXPECT_EQ(opts.rounding, RoundingMode::Round);
}

TEST(OptionsTest, SetAggregationMode) {
    CalculationOptions opts;
    opts.aggregation = AggregationMode::PerDay;
    EXPECT_EQ(opts.aggregation, AggregationMode::PerDay);
}
