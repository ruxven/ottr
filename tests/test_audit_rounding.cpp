#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "parser.hpp"
#include "validate.hpp"
#include "engine.hpp"
#include "report.hpp"
#include "model.hpp"

using namespace ottr;

class AuditRoundingTest : public ::testing::Test {
protected:
    // Parse a timecard and run the engine over it. Returns the aggregation so
    // the test can inspect both the raw detail and the rendered trail.
    Aggregation run(const std::string& content) {
        std::istringstream iss(content);
        ParserLogger log;
        World world;
        EXPECT_TRUE(parse_istream(iss, "test", world, log));
        EXPECT_TRUE(log.empty_err());
        EXPECT_TRUE(world.options.validate(error_)) << error_;

        Validator v;
        EXPECT_TRUE(v.validate_world("test", world));

        EngineOptions opts;
        opts.calc_options = world.options;

        Aggregation agg;
        std::string error;
        EXPECT_TRUE(process_world(world, opts, agg, error));
        return agg;
    }

    std::string error_;
};

// The exact proportional share of each charge is recorded so the audit trail
// can show what the split would have been without any rounding.
TEST_F(AuditRoundingTest, ExactShareIsRecorded) {
    std::string content = R"(
cn 1234.a "Charge A" 100
cn 1234.b "Charge B" 100
task alpha "Alpha Task"
wt alpha 1234.a 3
wt alpha 1234.b 1
opt audit on
opt rounding round
day 09/02
log 9.5 alpha "work"
log 11.0
)";

    Aggregation agg = run(content);
    ASSERT_EQ(agg.audit.size(), 2u);

    // 1.5 hours over a 3:1 split is 1.125h and 0.375h exactly.
    EXPECT_TRUE(agg.audit[0].rounding.applicable);
    EXPECT_EQ(agg.audit[0].rounding.ideal_numer, 45);
    EXPECT_EQ(agg.audit[0].rounding.ideal_denom, 4);
    EXPECT_EQ(agg.audit[0].rounding.rounded_ticks, 11);
    EXPECT_EQ(agg.audit[0].rounding.adjust_ticks, 0);

    EXPECT_TRUE(agg.audit[1].rounding.applicable);
    EXPECT_EQ(agg.audit[1].rounding.ideal_numer, 15);
    EXPECT_EQ(agg.audit[1].rounding.ideal_denom, 4);
    EXPECT_EQ(agg.audit[1].rounding.rounded_ticks, 4);
    EXPECT_EQ(agg.audit[1].rounding.adjust_ticks, 0);
}

// The rendered trail shows the exact share, what rounding did, what the
// remainder policy moved, and the net shift. This is the whole point: the
// automatic 0.1h adjustment must not be silent.
TEST_F(AuditRoundingTest, RenderAuditShowsRoundingBreakdown) {
    std::string content = R"(
cn 1234.a "Charge A" 100
cn 1234.b "Charge B" 100
task alpha "Alpha Task"
wt alpha 1234.a 3
wt alpha 1234.b 1
opt audit on
opt rounding round
day 09/02
log 9.5 alpha "work"
log 11.0
)";

    Aggregation agg = run(content);
    std::string out = render_audit(agg);

    EXPECT_NE(out.find("09/02 task alpha -> 1234.a 1.1"), std::string::npos);
    EXPECT_NE(out.find("09/02 task alpha -> 1234.b 0.4"), std::string::npos);
    EXPECT_NE(out.find("[exact 1.125, round -0.025, net -0.025]"), std::string::npos);
    EXPECT_NE(out.find("[exact 0.375, round +0.025, net +0.025]"), std::string::npos);
}

// When the rounding mode truncates and the remainder policy hands a whole
// quantum to one charge, both the remainder movement and the resulting net
// shift are shown.
TEST_F(AuditRoundingTest, RenderAuditShowsRemainderMovement) {
    std::string content = R"(
cn 1234.a "Charge A" 100
cn 1234.b "Charge B" 100
task alpha "Alpha Task"
wt alpha 1234.a 1
wt alpha 1234.b 1
opt audit on
opt rounding truncate
day 09/02
log 9.5 alpha "work"
log 11.0
)";

    Aggregation agg = run(content);
    std::string out = render_audit(agg);

    // 1.5h split evenly is 0.75h each; truncation drops 0.1h and the
    // remainder policy gives it back to the lower charge id.
    EXPECT_NE(out.find("[exact 0.75, round -0.05, rem +0.1, net +0.05]"), std::string::npos);
    EXPECT_NE(out.find("[exact 0.75, round -0.05, net -0.05]"), std::string::npos);
}

// The end-user workflow: an end-of-day allocation across weighted charge
// numbers, where the spreadsheet has to append a 0.1 hour to one time code by
// hand because the charge system only accepts tenths of an hour. ottr applies
// the same 0.1 automatically, moving it between the higher and lower weighted
// charge numbers. The audit must show where that 0.1 went.
TEST_F(AuditRoundingTest, DayEndAppendOnRoundedDistribution) {
    std::string content = R"(
cn 1234.a "Charge A" 100
cn 1234.b "Charge B" 100
task alpha "Alpha Task"
wt alpha 1234.a 3
wt alpha 1234.b 1
opt audit on
opt rounding round
opt aggregation per_day
day 09/02
log 9.0 alpha "work"
log 10.0
)";

    Aggregation agg = run(content);
    ASSERT_EQ(agg.audit.size(), 2u);

    // 1.0h split 3:1 is exactly 0.75h and 0.25h. Rounding to the tenth
    // sends both shares up by 0.05, so the rounded shares total 1.1h and
    // 0.1h has to come back off one of them.
    EXPECT_EQ(agg.audit[0].rounding.ideal_numer, 30);
    EXPECT_EQ(agg.audit[0].rounding.ideal_denom, 4);
    EXPECT_EQ(agg.audit[0].rounding.rounded_ticks, 8);
    EXPECT_EQ(agg.audit[0].rounding.adjust_ticks, -1);
    EXPECT_EQ(agg.audit[0].ticks, 7);

    EXPECT_EQ(agg.audit[1].rounding.ideal_numer, 10);
    EXPECT_EQ(agg.audit[1].rounding.ideal_denom, 4);
    EXPECT_EQ(agg.audit[1].rounding.rounded_ticks, 3);
    EXPECT_EQ(agg.audit[1].rounding.adjust_ticks, 0);
    EXPECT_EQ(agg.audit[1].ticks, 3);

    // The day still totals 1.0h: the 0.1 moved between charge numbers
    // instead of appearing out of nowhere.
    EXPECT_EQ(agg.totals_by_charge["1234.a"], 7);
    EXPECT_EQ(agg.totals_by_charge["1234.b"], 3);

    std::string out = render_audit(agg);
    // 1234.a was rounded up to 0.8 then gave the 0.1 back, landing 0.05 under
    // its exact 0.75 share.
    EXPECT_NE(out.find("[exact 0.75, round +0.05, rem -0.1, net -0.05]"), std::string::npos);
    // 1234.b was rounded up to 0.3 and kept it, landing 0.05 over its share.
    EXPECT_NE(out.find("[exact 0.25, round +0.05, net +0.05]"), std::string::npos);
}

// The charge that gives the 0.1 back is chosen by priority, so an end user
// can steer the automatic append onto the same time code their spreadsheet
// appends it to.
TEST_F(AuditRoundingTest, PriorityDecidesWhichChargeGivesBackTheTenth) {
    std::string content = R"(
cn 1234.a "Charge A" 100 1
cn 1234.b "Charge B" 100
task alpha "Alpha Task"
wt alpha 1234.a 3
wt alpha 1234.b 1
opt audit on
opt rounding round
opt aggregation per_day
day 09/02
log 9.0 alpha "work"
log 10.0
)";

    Aggregation agg = run(content);
    ASSERT_EQ(agg.audit.size(), 2u);

    // 1234.a now has the higher priority, so it keeps its rounded-up 0.8 and
    // 1234.b gives the 0.1 back instead.
    EXPECT_EQ(agg.audit[0].ticks, 8);
    EXPECT_EQ(agg.audit[0].rounding.adjust_ticks, 0);
    EXPECT_EQ(agg.audit[1].ticks, 2);
    EXPECT_EQ(agg.audit[1].rounding.adjust_ticks, -1);

    EXPECT_EQ(agg.totals_by_charge["1234.a"], 8);
    EXPECT_EQ(agg.totals_by_charge["1234.b"], 2);

    std::string out = render_audit(agg);
    EXPECT_NE(out.find("1234.a 0.8  [exact 0.75, round +0.05, net +0.05]"), std::string::npos);
    EXPECT_NE(out.find("1234.b 0.2  [exact 0.25, round +0.05, rem -0.1, net -0.05]"), std::string::npos);
}

// A split that lands exactly on the quantum boundary needs no explanation.
TEST_F(AuditRoundingTest, ExactSplitHasNoBreakdown) {
    std::string content = R"(
cn 1234.a "Charge A" 100
cn 1234.b "Charge B" 100
task alpha "Alpha Task"
wt alpha 1234.a 1
wt alpha 1234.b 1
opt audit on
day 09/02
log 9.5 alpha "work"
log 10.5
)";

    Aggregation agg = run(content);
    std::string out = render_audit(agg);

    EXPECT_NE(out.find("09/02 task alpha -> 1234.a 0.5"), std::string::npos);
    EXPECT_EQ(out.find("["), std::string::npos);
}

// A direct charge is not weighted, so nothing rounds and no breakdown shows.
TEST_F(AuditRoundingTest, DirectChargeHasNoBreakdown) {
    std::string content = R"(
cn 1234.a "Charge A" 100
opt audit on
day 09/02
log 9.5 1234.a "work"
log 11.0
)";

    Aggregation agg = run(content);
    ASSERT_EQ(agg.audit.size(), 1u);
    EXPECT_EQ(agg.audit[0].note, "direct");
    EXPECT_EQ(agg.audit[0].rounding.applicable, true);
    // A direct charge is exact: the share equals the slice, nothing moved.
    EXPECT_EQ(agg.audit[0].rounding.adjust_ticks, 0);
    EXPECT_EQ(agg.audit[0].rounding.clamped_ticks, 0);

    std::string out = render_audit(agg);
    EXPECT_NE(out.find("09/02 direct 1234.a -> 1234.a 1.5"), std::string::npos);
    EXPECT_EQ(out.find("["), std::string::npos);
}

// A budget cap that trims a charge is reported as its own adjustment, on top
// of the rounding shift.
TEST_F(AuditRoundingTest, RenderAuditShowsBudgetCap) {
    std::string content = R"(
cn 1234.a "Charge A" 0.5
cn 1234.b "Charge B" 100
task alpha "Alpha Task"
wt alpha 1234.a 1
wt alpha 1234.b 1
opt audit on
opt budget cap
day 09/02
log 9.5 alpha "work"
log 11.0
)";

    Aggregation agg = run(content);
    std::string out = render_audit(agg);

    // 1234.a has 0.5h of budget. Its exact share is 0.75h; truncation gives
    // 0.7h, then the cap trims a further 0.2h, leaving 0.5h.
    EXPECT_NE(out.find("1234.a 0.5"), std::string::npos);
    EXPECT_NE(out.find("[exact 0.75, round -0.05, cap -0.2, net -0.25]"), std::string::npos);
    // The remainder quantum goes to the charge with the larger remaining
    // budget, and the leftover is re-allocated on a second pass.
    EXPECT_NE(out.find("1234.b 0.8"), std::string::npos);
    EXPECT_NE(out.find("[exact 0.75, round -0.05, rem +0.1, net +0.05]"), std::string::npos);
    EXPECT_NE(out.find("1234.b 0.2"), std::string::npos);
    EXPECT_EQ(agg.totals_by_charge["1234.a"], 5);
    EXPECT_EQ(agg.totals_by_charge["1234.b"], 10);
}
