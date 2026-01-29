#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "parser.hpp"
#include "validate.hpp"
#include "model.hpp"

using namespace ottr;

TEST(TaskParser, HappyPath) {
    std::string txt = "task t \"test task\"";
    int line = 99;
    ParserLogger log;
    std::string err_str;
    World w;
    std::string source_name = "mem:test";
    TaskDirectiveParser task_parser(source_name,w,log);

    auto tokens = tokenize_line(txt);
    bool output = task_parser.parse(line, tokens);
    EXPECT_EQ(w.tasks["t"].description, "test task");
    EXPECT_TRUE(output);
}
