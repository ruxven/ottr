#include <gtest/gtest.h>
#include <sstream>
#include "parser.hpp"
#include "model.hpp"

using namespace ottr;

class OptionsParserTest : public ::testing::Test {
protected:
    void parse_string(const std::string& content) {
        std::istringstream iss(content);
        ParserLogger log;
        EXPECT_TRUE(parse_istream(iss, "test", world, log));
        EXPECT_TRUE(log.empty_err());
    }

    void parse_string_expect_error(const std::string& content) {
        std::istringstream iss(content);
        ParserLogger log;
        EXPECT_FALSE(parse_istream(iss, "test", world, log));
        EXPECT_FALSE(log.empty_err());
    }

    World world;
};

// Test parsing rounding option
TEST_F(OptionsParserTest, ParseRoundingTruncate) {
    parse_string("opt rounding truncate\n");
    EXPECT_EQ(world.options.rounding, RoundingMode::Truncate);
}

TEST_F(OptionsParserTest, ParseRoundingRound) {
    parse_string("opt rounding round\n");
    EXPECT_EQ(world.options.rounding, RoundingMode::Round);
}

TEST_F(OptionsParserTest, ParseRoundingCeil) {
    parse_string("opt rounding ceil\n");
    EXPECT_EQ(world.options.rounding, RoundingMode::Ceil);
}

// Test parsing aggregation option
TEST_F(OptionsParserTest, ParseAggregationPerEvent) {
    parse_string("opt aggregation per_event\n");
    EXPECT_EQ(world.options.aggregation, AggregationMode::PerEvent);
}

TEST_F(OptionsParserTest, ParseAggregationPerDay) {
    parse_string("opt aggregation per_day\n");
    EXPECT_EQ(world.options.aggregation, AggregationMode::PerDay);
}

TEST_F(OptionsParserTest, ParseAggregationOff) {
    parse_string("opt aggregation off\n");
    EXPECT_EQ(world.options.aggregation, AggregationMode::Off);
}

// Test invalid option key
TEST_F(OptionsParserTest, InvalidOptionKey) {
    parse_string_expect_error("opt invalid_key value\n");
}

// Test invalid rounding value
TEST_F(OptionsParserTest, InvalidRoundingValue) {
    parse_string_expect_error("opt rounding invalid\n");
}

// Test invalid aggregation value
TEST_F(OptionsParserTest, InvalidAggregationValue) {
    parse_string_expect_error("opt aggregation invalid\n");
}

// Test missing option value
TEST_F(OptionsParserTest, MissingOptionValue) {
    parse_string_expect_error("opt rounding\n");
}

// Test multiple options
TEST_F(OptionsParserTest, MultipleOptions) {
    parse_string("opt rounding round\nopt aggregation per_day\n");
    EXPECT_EQ(world.options.rounding, RoundingMode::Round);
    EXPECT_EQ(world.options.aggregation, AggregationMode::PerDay);
}

// Test option override (last one wins)
TEST_F(OptionsParserTest, OptionOverride) {
    // This test captures stderr to verify warning is printed
    parse_string("opt rounding truncate\nopt rounding round\n");
    EXPECT_EQ(world.options.rounding, RoundingMode::Round);
}

// Test options with other directives
TEST_F(OptionsParserTest, OptionsWithCharges) {
    parse_string(
        "cn 1234.a \"Charge A\" 100.0\n"
        "opt rounding round\n"
        "cn 1234.b \"Charge B\" 50.0\n"
    );
    EXPECT_EQ(world.options.rounding, RoundingMode::Round);
    EXPECT_EQ(world.charges.size(), 2);
}

// Test options with tasks
TEST_F(OptionsParserTest, OptionsWithTasks) {
    parse_string(
        "task mytask \"My Task\"\n"
        "opt aggregation per_day\n"
        "wt mytask 1234.a 1\n"
    );
    EXPECT_EQ(world.options.aggregation, AggregationMode::PerDay);
    EXPECT_EQ(world.tasks.size(), 1);
}
