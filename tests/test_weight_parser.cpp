#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "parser.hpp"
#include "validate.hpp"
#include "model.hpp"

using namespace ottr;

TEST(WeightParser, HappyPath) {
    std::string txt = "wt t C3 5";
    int line = 99;
    ParserLogger log;
    std::string err_str;
    World w;
    Task t;
    t.name = "t";
    t.description = "test task";
    w.tasks.emplace("t",t);
    std::string source_name = "mem:test";
    WeightDirectiveParser wt_parser(source_name,w,log);

    auto tokens = tokenize_line(txt);
    bool output = wt_parser.parse(line, tokens);
    ASSERT_EQ(w.tasks["t"].description, "test task");
    EXPECT_EQ(w.tasks["t"].weights["C3"], 5);
    EXPECT_EQ(w.tasks["t"].weights.size(), 1);
    EXPECT_TRUE(output);
}
