#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "parser.hpp"
#include "validate.hpp"
#include "model.hpp"

using namespace ottr;

TEST(Parser, HappyPath) {
    std::string txt =
        "cn 1234.a \"Charge A\" 10 1\n"
        "cn 1234.b \"Charge B\" 5\n"
        "task work \"General work\"\n"
        "wt work 1234.a 2\n"
        "wt work 1234.b 1\n"
        "day 09/01\n"
        "log 9.0 work\n"
        "log 12.0\n";
    ParserLogger log; std::string err_str;
    World w; std::istringstream iss(txt);
    ASSERT_TRUE(parse_istream(iss, "mem:parser_ok", w, log)) << err_str;
    EXPECT_TRUE(log.empty_err());
    EXPECT_EQ(w.charges.size(), 2u);
    EXPECT_EQ(w.tasks.size(), 1u);
    EXPECT_EQ(w.days.size(), 1u);
    ASSERT_EQ(w.days[0].logs.size(), 2u);
    EXPECT_EQ(w.days[0].logs[0].time_ticks, 90);
    EXPECT_EQ(w.days[0].logs[1].time_ticks, 120);
}

TEST(Parser, Errors) {
    {
        std::string txt = "task t \"x\"\ncn 1 \"d\" bad\n";
        ParserLogger log; std::string err_str; World w; std::istringstream iss(txt);
        EXPECT_FALSE(parse_istream(iss, "mem:parser_err1", w, log));
        EXPECT_NE(log.find_err("invalid hours in cn"), std::string::npos);
    }
    {
        std::string txt = "log 9.0 t\n";
        ParserLogger log; std::string err_str; World w; std::istringstream iss(txt);
        EXPECT_FALSE(parse_istream(iss, "mem:parser_err2", w, log));
        EXPECT_NE(log.find_err("log before any day declared"), std::string::npos);
    }
    {
        std::string txt = "day 09/01\nlog 10.0\nlog 9.0\n";
        ParserLogger log; std::string err_str; World w; std::istringstream iss(txt);
        EXPECT_FALSE(parse_istream(iss, "mem:parser_err3", w, log));
        EXPECT_NE(log.find_err("non-increasing log time"), std::string::npos);
    }
    {
        std::string txt = "day 09/02\nlog 9.0 x\n";
        ParserLogger log; std::string err_str; World w; std::istringstream iss(txt);
        EXPECT_FALSE(parse_istream(iss, "mem:parser_err4", w, log));
        EXPECT_NE(log.find_err("file ended but last day not closed"), std::string::npos);
    }
}

TEST(Validator, UnknownRefs) {
    std::string txt =
        "task t \"x\"\n"
        "wt t C1 1\n"
        "day 09/01\n"
        "log 9.0 t\n"
        "log 10.0\n";
    ParserLogger log; std::string err_str; World w; std::istringstream iss(txt);
    ASSERT_TRUE(parse_istream(iss, "mem:val", w, log));
    std::string verr = validate_world("mem:val", w);
    EXPECT_FALSE(log.empty_err());
    EXPECT_NE(log.find_err("wt references unknown charge: C1"), std::string::npos);
}
