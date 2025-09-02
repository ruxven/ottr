#include <gtest/gtest.h>
#include "util.hpp"
#include "parser.hpp"

using namespace ottr;

TEST(Utils, HoursToTicks) {
    EXPECT_EQ(hours_to_ticks(0.0), 0);
    EXPECT_EQ(hours_to_ticks(0.09), 0);
    EXPECT_EQ(hours_to_ticks(0.1), 1);
    EXPECT_EQ(hours_to_ticks(1.9999999), 19);
    EXPECT_EQ(hours_to_ticks(2.0), 20);
}

TEST(Utils, ParseMMDD) {
    Date d{};
    EXPECT_TRUE(parse_mmdd("09/02", d));
    EXPECT_EQ(d.ord, 902);
    EXPECT_FALSE(parse_mmdd("13/01", d));
    EXPECT_FALSE(parse_mmdd("09-02", d));
}

TEST(Tokenizer, QuotedAndEscapesAndComments) {
    auto toks = tokenize_line("cn 1234.a \"Desc with spaces\" 10 1 # comment");
    ASSERT_EQ(toks.size(), 5u);
    EXPECT_EQ(toks[0].text, "cn");
    EXPECT_TRUE(toks[2].quoted);
    EXPECT_EQ(toks[2].text, "Desc with spaces");

    auto toks2 = tokenize_line("task alpha \"say \\\"hi\\\"\"");
    ASSERT_EQ(toks2.size(), 3u);
    EXPECT_TRUE(toks2[2].quoted);
    EXPECT_EQ(toks2[2].text, "say \"hi\"");
}
