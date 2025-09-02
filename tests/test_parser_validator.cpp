#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include "parser.hpp"
#include "validate.hpp"
#include "model.hpp"

using namespace ottr;

static std::string write_temp_file(const std::string& rel_name, const std::string& contents) {
    std::string path = std::string("/tmp/") + rel_name;
    std::ofstream f(path);
    f << contents;
    f.close();
    return path;
}

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
    std::string err;
    World w; std::string path = write_temp_file("parser_ok.txt", txt);
    ASSERT_TRUE(parse_file(path, w, err)) << err;
    EXPECT_TRUE(err.empty());
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
        std::string err; World w; std::string path = write_temp_file("parser_err1.txt", txt);
        EXPECT_FALSE(parse_file(path, w, err));
        EXPECT_NE(err.find("invalid hours in cn"), std::string::npos);
    }
    {
        std::string txt = "log 9.0 t\n";
        std::string err; World w; std::string path = write_temp_file("parser_err2.txt", txt);
        EXPECT_FALSE(parse_file(path, w, err));
        EXPECT_NE(err.find("log before any day declared"), std::string::npos);
    }
    {
        std::string txt = "day 09/01\nlog 10.0\nlog 9.0\n";
        std::string err; World w; std::string path = write_temp_file("parser_err3.txt", txt);
        EXPECT_FALSE(parse_file(path, w, err));
        EXPECT_NE(err.find("non-increasing log time"), std::string::npos);
    }
    {
        std::string txt = "day 09/02\nlog 9.0 x\n";
        std::string err; World w; std::string path = write_temp_file("parser_err4.txt", txt);
        EXPECT_FALSE(parse_file(path, w, err));
        EXPECT_NE(err.find("file ended but last day not closed"), std::string::npos);
    }
}

TEST(Validator, UnknownRefs) {
    std::string txt =
        "task t \"x\"\n"
        "wt t C1 1\n"
        "day 09/01\n"
        "log 9.0 t\n"
        "log 10.0\n";
    std::string err; World w; std::string path = write_temp_file("val.txt", txt);
    ASSERT_TRUE(parse_file(path, w, err));
    std::string verr = validate_world(path, w);
    EXPECT_FALSE(verr.empty());
    EXPECT_NE(verr.find("wt references unknown charge: C1"), std::string::npos);
}
