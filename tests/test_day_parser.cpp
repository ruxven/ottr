#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "parser.hpp"
#include "validate.hpp"
#include "model.hpp"

using namespace ottr;

TEST(DayParser, HappyPath) {
    std::string txt = "day 09/11";
    int line = 99;
    ParserLogger log;
    std::string err_str;
    World w;
    std::string source_name = "mem:test";
    DayDirectiveParser day_parser(source_name,w,log);

    auto tokens = tokenize_line(txt);
    bool output = day_parser.parse(line, tokens);
    ASSERT_FALSE(w.days.empty());
    EXPECT_EQ(w.current_day().date.raw, "09/11");
    EXPECT_TRUE(output);
}
