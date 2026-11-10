#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "cli.hpp"

using namespace ottr;

namespace {

CliOptions parse_ok(const std::vector<std::string>& args) {
    CliOptions out;
    std::string error;
    EXPECT_EQ(parse_cli(args, out, error), CliStatus::Ok) << error;
    return out;
}

std::string parse_error(const std::vector<std::string>& args) {
    CliOptions out;
    std::string error;
    EXPECT_EQ(parse_cli(args, out, error), CliStatus::Error);
    return error;
}

} // namespace

TEST(Cli, HelpLongAndShort) {
    CliOptions out;
    std::string error;
    EXPECT_EQ(parse_cli({"--help"}, out, error), CliStatus::Help);
    EXPECT_EQ(parse_cli({"-h"}, out, error), CliStatus::Help);
}

TEST(Cli, HelpWinsOverOtherArguments) {
    CliOptions out;
    std::string error;
    EXPECT_EQ(parse_cli({"timecard.txt", "--help"}, out, error), CliStatus::Help);
    EXPECT_EQ(parse_cli({"--help", "timecard.txt"}, out, error), CliStatus::Help);
}

TEST(Cli, SingleFile) {
    CliOptions out = parse_ok({"timecard.txt"});
    EXPECT_EQ(out.path, "timecard.txt");
    EXPECT_FALSE(out.filter.has_start);
    EXPECT_FALSE(out.filter.has_end);
}

TEST(Cli, SingleDay) {
    CliOptions out = parse_ok({"timecard.txt", "09/02"});
    EXPECT_EQ(out.path, "timecard.txt");
    EXPECT_TRUE(out.filter.has_start);
    EXPECT_FALSE(out.filter.has_end);
    EXPECT_EQ(out.filter.start.raw, "09/02");
    EXPECT_EQ(out.filter.start.ord, 902);
}

TEST(Cli, DateRange) {
    CliOptions out = parse_ok({"timecard.txt", "09/02", "09/30"});
    EXPECT_TRUE(out.filter.has_start);
    EXPECT_TRUE(out.filter.has_end);
    EXPECT_EQ(out.filter.start.ord, 902);
    EXPECT_EQ(out.filter.end.ord, 930);
}

TEST(Cli, MissingFile) {
    EXPECT_NE(parse_error({}).find("missing FILE"), std::string::npos);
}

TEST(Cli, TooManyArguments) {
    EXPECT_NE(parse_error({"a", "09/02", "09/03", "09/04"}).find("too many"),
              std::string::npos);
}

TEST(Cli, UnknownOption) {
    EXPECT_NE(parse_error({"--bogus"}).find("unknown option"), std::string::npos);
}

TEST(Cli, InvalidDates) {
    EXPECT_NE(parse_error({"timecard.txt", "notadate"}).find("invalid START"),
              std::string::npos);
    EXPECT_NE(parse_error({"timecard.txt", "09/02", "13/40"}).find("invalid END"),
              std::string::npos);
}

TEST(Cli, DoubleDashAllowsDashFilename) {
    CliOptions out = parse_ok({"--", "--weird.txt"});
    EXPECT_EQ(out.path, "--weird.txt");
    EXPECT_FALSE(out.filter.has_start);
}

TEST(Cli, HelpTextDescribesRunModes) {
    const std::string text = help_text("ottr");
    EXPECT_NE(text.find("Usage:"), std::string::npos);
    EXPECT_NE(text.find("Run modes:"), std::string::npos);
    EXPECT_NE(text.find("--help"), std::string::npos);
    EXPECT_NE(text.find("FILE START END"), std::string::npos);
}

TEST(Cli, HelpTextLinksToReadme) {
    const std::string text = help_text("ottr");
    EXPECT_NE(text.find("https://github.com/ruxven/ottr/blob/main/README.md"),
              std::string::npos);
}
