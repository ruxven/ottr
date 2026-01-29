#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "parser.hpp"
#include "validate.hpp"
#include "model.hpp"

using namespace ottr;

TEST(LogParser, HappyPath) {
    std::string cn_line = "cn COFFEE \"Coffee Break\" 80";
    std::string task_line = "task coffee \"COFFEE\"";
    std::string day_line = "day 09/11";
    std::string log_line = "log 9.5 coffee";
    int line = 99;
    ParserLogger log;
    std::string err_str;
    World w;
    std::string source_name = "mem:test";
    ChargeDirectiveParser charge_parser(source_name,w,log);
    TaskDirectiveParser task_parser(source_name,w,log);
    DayDirectiveParser day_parser(source_name,w,log);
    LogDirectiveParser log_parser(source_name,w,log);

    {
        auto tokens = tokenize_line(cn_line);
        bool output = charge_parser.parse(line, tokens);
        ASSERT_TRUE(output) << "failed to parse charge number entry";
    }

    {
        auto tokens = tokenize_line(task_line);
        bool output = task_parser.parse(line, tokens);
        ASSERT_TRUE(output) << "failed to parse task entry";
    }

    {
        auto tokens = tokenize_line(day_line);
        bool output = day_parser.parse(line, tokens);
        ASSERT_TRUE(output) << "failed to parse day entry";
    }

    auto tokens = tokenize_line(log_line);
    bool output = log_parser.parse(line, tokens);
    EXPECT_TRUE(output) << "failed to parse log entry";
    ASSERT_EQ(w.days[0].logs.size(), 1u);
}
