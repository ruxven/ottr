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

    // Parse successfully and return the collected warnings.
    std::vector<std::string> parse_string_expect_warnings(const std::string& content) {
        std::istringstream iss(content);
        ParserLogger log;
        EXPECT_TRUE(parse_istream(iss, "test", world, log));
        EXPECT_TRUE(log.empty_err());
        EXPECT_FALSE(log.empty_warnings());
        return log.get_warnings();
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

// Remainder policy
TEST_F(OptionsParserTest, ParseRemainderPolicies) {
    parse_string("opt remainder priority\n");
    EXPECT_EQ(world.options.remainder, RemainderPolicy::Priority);

    parse_string("opt remainder round_robin\n");
    EXPECT_EQ(world.options.remainder, RemainderPolicy::RoundRobin);

    parse_string("opt remainder largest_frac\n");
    EXPECT_EQ(world.options.remainder, RemainderPolicy::LargestFrac);
}

TEST_F(OptionsParserTest, InvalidRemainderPolicy) {
    parse_string_expect_error("opt remainder sequential\n");
}

// Budget policy
TEST_F(OptionsParserTest, ParseBudgetPolicies) {
    parse_string("opt budget ignore\n");
    EXPECT_EQ(world.options.budget, BudgetPolicy::Ignore);

    parse_string("opt budget warn\n");
    EXPECT_EQ(world.options.budget, BudgetPolicy::Warn);

    parse_string("opt budget cap\n");
    EXPECT_EQ(world.options.budget, BudgetPolicy::Cap);
}

TEST_F(OptionsParserTest, ParseBudgetSpillWithOverflowCharge) {
    parse_string("opt budget spill\nopt overflow_charge 9999.z\n");
    EXPECT_EQ(world.options.budget, BudgetPolicy::Spill);
    EXPECT_EQ(world.options.overflow_charge, "9999.z");
}

TEST_F(OptionsParserTest, InvalidBudgetPolicy) {
    parse_string_expect_error("opt budget drop\n");
}

TEST_F(OptionsParserTest, QuotedOverflowChargeIsRejected) {
    parse_string_expect_error("opt overflow_charge \"9999.z\"\n");
}

// Granularity
TEST_F(OptionsParserTest, ParseGranularityTenths) {
    parse_string("opt granularity 0.1\n");
    EXPECT_EQ(world.options.granularity, 1);

    parse_string("opt granularity 0.5\n");
    EXPECT_EQ(world.options.granularity, 5);

    parse_string("opt granularity 1.0\n");
    EXPECT_EQ(world.options.granularity, 10);
}

TEST_F(OptionsParserTest, GranularityMustBeWholeTenths) {
    // Quarter hour cannot be represented in tenths-of-an-hour ticks.
    parse_string_expect_error("opt granularity 0.25\n");
}

TEST_F(OptionsParserTest, InvalidGranularity) {
    parse_string_expect_error("opt granularity 0\n");
    parse_string_expect_error("opt granularity -0.5\n");
    parse_string_expect_error("opt granularity abc\n");
    parse_string_expect_error("opt granularity 25.0\n");
}

// Audit flag
TEST_F(OptionsParserTest, ParseAuditFlag) {
    parse_string("opt audit on\n");
    EXPECT_TRUE(world.options.audit);

    parse_string("opt audit off\n");
    EXPECT_FALSE(world.options.audit);
}

TEST_F(OptionsParserTest, InvalidAuditFlag) {
    parse_string_expect_error("opt audit yes\n");
}

// Per-month aggregation
TEST_F(OptionsParserTest, ParseAggregationPerMonth) {
    parse_string("opt aggregation per_month\n");
    EXPECT_EQ(world.options.aggregation, AggregationMode::PerMonth);
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
    parse_string("opt rounding truncate\nopt rounding round\n");
    EXPECT_EQ(world.options.rounding, RoundingMode::Round);
}

// A repeated option warns even when the first value was the default. The old
// implementation compared against the default and stayed silent here.
TEST_F(OptionsParserTest, DuplicateOptionWarnsWhenFirstValueIsDefault) {
    auto warnings = parse_string_expect_warnings(
        "opt rounding truncate\nopt rounding round\n");
    ASSERT_EQ(warnings.size(), 1u);
    EXPECT_NE(warnings[0].find("rounding"), std::string::npos);
    EXPECT_NE(warnings[0].find("more than once"), std::string::npos);
    EXPECT_EQ(world.options.rounding, RoundingMode::Round);
}

TEST_F(OptionsParserTest, DuplicateOptionWarnsEvenWhenValueUnchanged) {
    auto warnings = parse_string_expect_warnings(
        "opt aggregation per_day\nopt aggregation per_day\n");
    ASSERT_EQ(warnings.size(), 1u);
    EXPECT_EQ(world.options.aggregation, AggregationMode::PerDay);
}

TEST_F(OptionsParserTest, DuplicateWarnsOncePerRepeat) {
    auto warnings = parse_string_expect_warnings(
        "opt rounding truncate\n"
        "opt rounding truncate\n"
        "opt rounding round\n");
    EXPECT_EQ(warnings.size(), 2u);
    EXPECT_EQ(world.options.rounding, RoundingMode::Round);
}

TEST_F(OptionsParserTest, DistinctOptionsDoNotWarn) {
    parse_string("opt rounding round\nopt aggregation per_day\n");
    EXPECT_EQ(world.options.rounding, RoundingMode::Round);
    EXPECT_EQ(world.options.aggregation, AggregationMode::PerDay);
}

// An invalid repeated value is an error, not an override, so no "last value
// wins" warning should accompany it.
TEST_F(OptionsParserTest, InvalidDuplicateReportsErrorWithoutOverrideWarning) {
    std::istringstream iss("opt rounding round\nopt rounding bogus\n");
    ParserLogger log;
    EXPECT_FALSE(parse_istream(iss, "test", world, log));
    EXPECT_FALSE(log.empty_err());
    EXPECT_TRUE(log.empty_warnings());
}

// Test CalculationOptions::validate
TEST_F(OptionsParserTest, ValidateAcceptsDefaultsAndEveryMode) {
    CalculationOptions opts;
    std::string error;
    EXPECT_TRUE(opts.validate(error));

    opts.rounding = RoundingMode::Round;
    opts.aggregation = AggregationMode::PerDay;
    EXPECT_TRUE(opts.validate(error));

    opts.rounding = RoundingMode::Ceil;
    opts.aggregation = AggregationMode::Off;
    EXPECT_TRUE(opts.validate(error));
}

TEST_F(OptionsParserTest, ValidateBudgetSpillRequiresOverflowCharge) {
    CalculationOptions opts;
    std::string error;
    opts.budget = BudgetPolicy::Spill;
    EXPECT_FALSE(opts.validate(error));
    EXPECT_NE(error.find("overflow_charge"), std::string::npos);

    opts.overflow_charge = "9999.z";
    EXPECT_TRUE(opts.validate(error));
}

TEST_F(OptionsParserTest, ValidateOverflowChargeOnlyWithSpill) {
    CalculationOptions opts;
    std::string error;
    opts.overflow_charge = "9999.z";
    EXPECT_FALSE(opts.validate(error));
    EXPECT_NE(error.find("spill"), std::string::npos);
}

TEST_F(OptionsParserTest, ValidateRejectsNonPositiveGranularity) {
    CalculationOptions opts;
    std::string error;
    opts.granularity = 0;
    EXPECT_FALSE(opts.validate(error));
}

TEST_F(OptionsParserTest, ValidateRejectsOutOfRangeEnum) {
    CalculationOptions opts;
    std::string error;
    opts.rounding = static_cast<RoundingMode>(99);
    EXPECT_FALSE(opts.validate(error));
    EXPECT_FALSE(error.empty());

    opts = CalculationOptions{};
    error.clear();
    opts.aggregation = static_cast<AggregationMode>(99);
    EXPECT_FALSE(opts.validate(error));
    EXPECT_FALSE(error.empty());
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
