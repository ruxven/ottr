#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <string>
#include "parser.hpp"
#include "validate.hpp"
#include "engine.hpp"
#include "report.hpp"

using namespace ottr;

TEST(Engine, DirectAndTaskAllocationAndFilter) {
    std::string txt =
        "cn C1 \"A\" 10 0\n"
        "cn C2 \"B\" 10 1\n"
        "task t \"task\"\n"
        "wt t C1 1\n"
        "wt t C2 1\n"
        "day 09/01\n"
        "log 9.0 t\n"
        "log 10.0 C1\n"
        "log 11.0\n"
        "day 09/02\n"
        "log 9.0 t\n"
        "log 10.5\n";
    std::string err; World w; std::istringstream iss(txt);
    ASSERT_TRUE(parse_istream(iss, "mem:eng", w, err)) << err;
    ASSERT_TRUE(validate_world("mem:eng", w).empty());

    EngineOptions opts{}; opts.filter.has_start=true; opts.filter.start.raw="09/01"; opts.filter.start.ord=901;
    Aggregation agg; std::string emsg; ASSERT_TRUE(process_world(w, opts, agg, emsg));
    EXPECT_EQ(agg.totals_by_charge["C1"], 15);
    EXPECT_EQ(agg.totals_by_charge["C2"], 5);

    opts.filter.has_end=true; opts.filter.end.raw="09/02"; opts.filter.end.ord=902;
    Aggregation agg2; ASSERT_TRUE(process_world(w, opts, agg2, emsg));
    // Day 09/02 adds 15 ticks split base 7/7 remainder 1 goes to C2 due to higher priority
    EXPECT_EQ(agg2.totals_by_charge["C1"], 22);
    EXPECT_EQ(agg2.totals_by_charge["C2"], 13);
}

TEST(Report, FullAndSingleDayTables) {
    std::string txt =
        "cn C1 \"A\" 2\n"
        "cn C2 \"B\" 1\n"
        "task t \"x\"\n"
        "wt t C1 1\n"
        "wt t C2 1\n"
        "day 09/01\n"
        "log 9.0 t\n"
        "log 10.0\n";
    std::string err; World w; std::istringstream iss(txt);
    ASSERT_TRUE(parse_istream(iss, "mem:rep", w, err)) << err;
    ASSERT_TRUE(validate_world("mem:rep", w).empty());

    EngineOptions opts{}; opts.filter.has_start=true; opts.filter.start.raw="09/01"; opts.filter.start.ord=901;
    Aggregation agg; std::string emsg; ASSERT_TRUE(process_world(w, opts, agg, emsg));

    std::string full = render_full_table(w, agg);
    std::ostringstream exp;
    exp << "| Charge | 09/01 | Total | Rem |\n";
    exp << "| C1     |   0.5 |   0.5 | 1.5 |\n";
    exp << "| C2     |   0.5 |   0.5 | 0.5 |\n";
    exp << "| Total  |   1.0 |   1.0 |     |\n";
    EXPECT_EQ(full, exp.str());

    std::string single = render_single_day_table(w, agg, agg.dates_in_order[0]);
    std::ostringstream exp2;
    exp2 << "| Charge | 09/01 |\n";
    exp2 << "| C1     |   0.5 |\n";
    exp2 << "| C2     |   0.5 |\n";
    exp2 << "| Total  |   1.0 |\n";
    EXPECT_EQ(single, exp2.str());
}

TEST(Smoke, DemoTimecardParsesIfPresent) {
#ifdef OTTR_SOURCE_DIR
    std::string demo = std::string(OTTR_SOURCE_DIR) + "/ottr/demo_timecard.txt";
    std::ifstream f(demo);
    if (!f.good()) GTEST_SKIP() << "demo_timecard.txt not present";
    World w; std::string err; ASSERT_TRUE(parse_file(demo, w, err)) << err;
    EXPECT_TRUE(validate_world(demo, w).empty());
#endif
}
