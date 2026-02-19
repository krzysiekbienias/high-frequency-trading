#include <gtest/gtest.h>

#include "parser/tokenize.hpp"

TEST(TokenizeTests, EmptyString_ReturnsSingleEmptyToken) {
    auto tokens = tokenize("");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "");
}

TEST(TokenizeTests, NoComma_ReturnsSingleTrimmedToken) {
    auto tokens = tokenize("  ABC  ");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "ABC");
}

TEST(TokenizeTests, SimpleSplit_SplitsIntoTokens) {
    auto tokens = tokenize("A,B,C");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "A");
    EXPECT_EQ(tokens[1], "B");
    EXPECT_EQ(tokens[2], "C");
}

TEST(TokenizeTests, TrimsWhitespaceAroundTokens) {
    auto tokens = tokenize("  A ,  B,   C   ");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "A");
    EXPECT_EQ(tokens[1], "B");
    EXPECT_EQ(tokens[2], "C");
}

TEST(TokenizeTests, PreservesEmptyTokens_InMiddle) {
    auto tokens = tokenize("A,,C");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "A");
    EXPECT_EQ(tokens[1], "");
    EXPECT_EQ(tokens[2], "C");
}

TEST(TokenizeTests, PreservesEmptyTokens_Leading) {
    auto tokens = tokenize(",A,B");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "");
    EXPECT_EQ(tokens[1], "A");
    EXPECT_EQ(tokens[2], "B");
}

TEST(TokenizeTests, PreservesEmptyTokens_TrailingComma) {
    auto tokens = tokenize("A,B,");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "A");
    EXPECT_EQ(tokens[1], "B");
    EXPECT_EQ(tokens[2], "");
}

TEST(TokenizeTests, HandlesNewlineLF_StripsAtEndOnly) {
    auto tokens = tokenize("A,B,C\n");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "A");
    EXPECT_EQ(tokens[1], "B");
    EXPECT_EQ(tokens[2], "C");
}

TEST(TokenizeTests, HandlesNewlineCRLF_StripsAtEndOnly) {
    auto tokens = tokenize("A,B,C\r\n");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "A");
    EXPECT_EQ(tokens[1], "B");
    EXPECT_EQ(tokens[2], "C");
}



TEST(TokenizeTests, TypicalOrderLine_Produces8Tokens) {
    auto tokens = tokenize("N,2,00000002,XYZ,L,B,104.53,100");
    ASSERT_EQ(tokens.size(), 8u);
    EXPECT_EQ(tokens[0], "N");
    EXPECT_EQ(tokens[1], "2");
    EXPECT_EQ(tokens[2], "00000002");
    EXPECT_EQ(tokens[3], "XYZ");
    EXPECT_EQ(tokens[4], "L");
    EXPECT_EQ(tokens[5], "B");
    EXPECT_EQ(tokens[6], "104.53");
    EXPECT_EQ(tokens[7], "100");
}