#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "engine.hpp"
#include "generate.hpp"
#include "parser.hpp"
#include "validate.hpp"

using namespace ottr;

TEST(Generate, ParseFullDate) {
    CalendarDate date;
    ASSERT_TRUE(parse_calendar_date("2026/09/01", 2000, date));
    EXPECT_EQ(date.year, 2026);
    EXPECT_EQ(date.month, 9);
    EXPECT_EQ(date.day, 1);
}

TEST(Generate, ParseShortDateUsesDefaultYear) {
    CalendarDate date;
    ASSERT_TRUE(parse_calendar_date("09/01", 2026, date));
    EXPECT_EQ(date.year, 2026);
    EXPECT_EQ(date.month, 9);
    EXPECT_EQ(date.day, 1);

    ASSERT_TRUE(parse_calendar_date("9/1", 2026, date));
    EXPECT_EQ(date.month, 9);
    EXPECT_EQ(date.day, 1);
}

TEST(Generate, ParseRejectsInvalidDates) {
    CalendarDate date;
    EXPECT_FALSE(parse_calendar_date("", 2026, date));
    EXPECT_FALSE(parse_calendar_date("2026/09", 2026, date));
    EXPECT_FALSE(parse_calendar_date("2026/09/01/02", 2026, date));
    EXPECT_FALSE(parse_calendar_date("2026/13/01", 2026, date));
    EXPECT_FALSE(parse_calendar_date("2026/00/01", 2026, date));
    EXPECT_FALSE(parse_calendar_date("2026/09/00", 2026, date));
    EXPECT_FALSE(parse_calendar_date("2026/09/31", 2026, date));
    EXPECT_FALSE(parse_calendar_date("2026/02/30", 2026, date));
    EXPECT_FALSE(parse_calendar_date("2026/ab/01", 2026, date));
    EXPECT_FALSE(parse_calendar_date("2026/09/01x", 2026, date));
}

TEST(Generate, ParseRespectsLeapYears) {
    CalendarDate date;
    EXPECT_TRUE(parse_calendar_date("2024/02/29", 2026, date));
    EXPECT_FALSE(parse_calendar_date("2025/02/29", 2026, date));
    EXPECT_FALSE(parse_calendar_date("1900/02/29", 2026, date));
    EXPECT_TRUE(parse_calendar_date("2000/02/29", 2026, date));
}

TEST(Generate, RangeRequiresOrderedDates) {
    CalendarDate start;
    CalendarDate end;
    std::string error;
    EXPECT_TRUE(parse_generate_range("2026/09/01", "2026/09/30", 2026, start,
                                     end, error));

    EXPECT_FALSE(parse_generate_range("2026/09/30", "2026/09/01", 2026, start,
                                      end, error));
    EXPECT_NE(error.find("before start date"), std::string::npos);

    EXPECT_TRUE(parse_generate_range("2026/09/01", "2026/09/01", 2026, start,
                                     end, error));
}

TEST(Generate, RangeReportsWhichDateIsInvalid) {
    CalendarDate start;
    CalendarDate end;
    std::string error;
    EXPECT_FALSE(parse_generate_range("bad", "2026/09/01", 2026, start, end,
                                      error));
    EXPECT_NE(error.find("invalid start date"), std::string::npos);

    EXPECT_FALSE(parse_generate_range("2026/09/01", "bad", 2026, start, end,
                                      error));
    EXPECT_NE(error.find("invalid end date"), std::string::npos);
}

TEST(Generate, SkipsWeekendsAndEmitsBlocks) {
    CalendarDate start;
    CalendarDate end;
    std::string error;
    ASSERT_TRUE(parse_generate_range("2026/09/04", "2026/09/08", 2026, start,
                                     end, error));

    const std::string expected =
        "# 2026/09/04 - 2026/09/08\n"
        "\n"
        "# day 247\n"
        "day 09/04\n"
        "log 9.0\n"
        "log 17.0\n"
        "\n"
        "# week 37\n"
        "\n"
        "# day 250\n"
        "day 09/07\n"
        "log 9.0\n"
        "log 17.0\n"
        "\n"
        "# day 251\n"
        "day 09/08\n"
        "log 9.0\n"
        "log 17.0\n";
    EXPECT_EQ(generate_timecard(start, end), expected);
}

TEST(Generate, WeekendOnlyRangeHasNoDayBlocks) {
    CalendarDate start;
    CalendarDate end;
    std::string error;
    ASSERT_TRUE(parse_generate_range("2026/09/05", "2026/09/06", 2026, start,
                                     end, error));
    EXPECT_EQ(generate_timecard(start, end),
              "# 2026/09/05 - 2026/09/06\n");
}

TEST(Generate, WeekCommentOnlyOnMondays) {
    CalendarDate start;
    CalendarDate end;
    std::string error;
    ASSERT_TRUE(parse_generate_range("2026/09/01", "2026/09/07", 2026, start,
                                     end, error));
    const std::string text = generate_timecard(start, end);
    const size_t first = text.find("# week");
    ASSERT_NE(first, std::string::npos);
    EXPECT_EQ(first, text.find("# week 37\n"));
    // The only week comment belongs to Monday 09/07.
    EXPECT_EQ(text.find("# week", first + 1), std::string::npos);
}

TEST(Generate, IsoWeekRollsOverAtTheNewYear) {
    CalendarDate start;
    CalendarDate end;
    std::string error;

    ASSERT_TRUE(parse_generate_range("2020/12/28", "2020/12/28", 2026, start,
                                     end, error));
    EXPECT_NE(generate_timecard(start, end).find("# week 53\n"),
              std::string::npos);

    ASSERT_TRUE(parse_generate_range("2021/01/01", "2021/01/04", 2026, start,
                                     end, error));
    const std::string text = generate_timecard(start, end);
    EXPECT_NE(text.find("# week 1\n"), std::string::npos);
    EXPECT_EQ(text.find("# week 53\n"), std::string::npos);
}

TEST(Generate, DayOfYearHandlesLeapYears) {
    CalendarDate start;
    CalendarDate end;
    std::string error;
    ASSERT_TRUE(parse_generate_range("2020/12/31", "2020/12/31", 2026, start,
                                     end, error));
    EXPECT_NE(generate_timecard(start, end).find("# day 366\n"),
              std::string::npos);
}

TEST(Generate, HeaderUsesTheRequestedRange) {
    CalendarDate start;
    CalendarDate end;
    std::string error;
    ASSERT_TRUE(parse_generate_range("2026/09/01", "2026/09/30", 2026, start,
                                     end, error));
    const std::string text = generate_timecard(start, end);
    EXPECT_EQ(text.rfind("# 2026/09/01 - 2026/09/30\n\n", 0), 0u);
}

TEST(Generate, GeneratedSkeletonParsesAsATimecard) {
    CalendarDate start;
    CalendarDate end;
    std::string error;
    ASSERT_TRUE(parse_generate_range("2026/09/01", "2026/09/08", 2026, start,
                                     end, error));

    std::istringstream iss(generate_timecard(start, end));
    ParserLogger log;
    World world;
    ASSERT_TRUE(parse_istream(iss, "generated", world, log));
    EXPECT_TRUE(log.empty_err());

    Validator validator;
    ASSERT_TRUE(validator.validate_world("generated", world));

    EngineOptions opts;
    Aggregation agg;
    ASSERT_TRUE(process_world(world, opts, agg, error)) << error;
    // 09/01 through 09/08 contains six weekdays.
    EXPECT_EQ(agg.dates_in_order.size(), 6u);
}
