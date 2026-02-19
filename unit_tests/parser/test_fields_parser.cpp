#include <gtest/gtest.h>

#include "parser/fields_parser.hpp"

TEST(FieldParsersTests, ParseInt64Strict_AcceptsValidIntegers) {
    auto v1 = parseInt64Strict("0");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, 0);

    auto v2 = parseInt64Strict("12345");
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(*v2, 12345);

    auto v3 = parseInt64Strict("-7");
    ASSERT_TRUE(v3.has_value());
    EXPECT_EQ(*v3, -7);
}

TEST(FieldParsersTests, ParseInt64Strict_RejectsEmptyOrJunk) {
    EXPECT_FALSE(parseInt64Strict("").has_value());
    EXPECT_FALSE(parseInt64Strict("   ").has_value());

    EXPECT_FALSE(parseInt64Strict("12x").has_value());
    EXPECT_FALSE(parseInt64Strict("x12").has_value());
    EXPECT_FALSE(parseInt64Strict("1 2").has_value());

    // critical: must reject decimals / trailing dot
    EXPECT_FALSE(parseInt64Strict("100.").has_value());
    EXPECT_FALSE(parseInt64Strict("100.0").has_value());
    EXPECT_FALSE(parseInt64Strict("100.00").has_value());
}

TEST(FieldParsersTests, ParseOrderId_AcceptsPositiveOnly) {
    EXPECT_FALSE(parseOrderId("0").has_value());
    EXPECT_FALSE(parseOrderId("-1").has_value());

    auto v = parseOrderId("42");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 42);
}

TEST(FieldParsersTests, ParseTimestamp_AcceptsZeroOrPositiveOnly) {
    auto v0 = parseTimestamp("0");
    ASSERT_TRUE(v0.has_value());
    EXPECT_EQ(*v0, 0);

    auto v = parseTimestamp("999");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 999);

    EXPECT_FALSE(parseTimestamp("-1").has_value());
}

TEST(FieldParsersTests, ParseQuantity_AcceptsPositiveOnly) {
    EXPECT_FALSE(parseQuantity("0").has_value());
    EXPECT_FALSE(parseQuantity("-10").has_value());
    EXPECT_FALSE(parseQuantity("10.").has_value());
    EXPECT_FALSE(parseQuantity("10.7").has_value());

    auto v = parseQuantity("100");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 100);
}

TEST(FieldParsersTests, ParseSide_AcceptsBOrSOnly) {
    auto b = parseSide("B");
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(*b, domain::Side::Buy);

    auto s = parseSide("S");
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(*s, domain::Side::Sell);

    EXPECT_FALSE(parseSide("").has_value());
    EXPECT_FALSE(parseSide("BUY").has_value());
    EXPECT_FALSE(parseSide("b").has_value()); // strict for now
}

TEST(FieldParsersTests, ParseOrderType_AcceptsMLIOnly) {
    auto m = parseOrderType("M");
    ASSERT_TRUE(m.has_value());
    EXPECT_EQ(*m, domain::OrderType::Market);

    auto l = parseOrderType("L");
    ASSERT_TRUE(l.has_value());
    EXPECT_EQ(*l, domain::OrderType::Limit);

    auto i = parseOrderType("I");
    ASSERT_TRUE(i.has_value());
    EXPECT_EQ(*i, domain::OrderType::IOC);

    EXPECT_FALSE(parseOrderType("").has_value());
    EXPECT_FALSE(parseOrderType("X").has_value());
    EXPECT_FALSE(parseOrderType("m").has_value()); // strict for now
}

TEST(FieldParsersTests, ParsePriceCents_AcceptsTwoDecimalsOnly) {

    auto p1 = parsePriceCents("104.53");
    ASSERT_TRUE(p1.has_value());
    EXPECT_EQ(*p1, 10453);

    auto p2 = parsePriceCents("000.01");
    ASSERT_TRUE(p2.has_value());
    EXPECT_EQ(*p2, 1);

    auto p3 = parsePriceCents("10.90");
    ASSERT_TRUE(p3.has_value());
    EXPECT_EQ(*p3, 1090);
}


TEST(FieldParsersTests, PriceMustBePositive) {
    //in spec states that it is allowed for OrderType Market so must be accepted on parsing level.
    auto p1 = parsePriceCents("0.00");
    ASSERT_TRUE(p1.has_value());

}

TEST(FieldParsersTests, ParsePriceCents_RejectsInvalidFormats) {
    EXPECT_FALSE(parsePriceCents("").has_value());
    EXPECT_FALSE(parsePriceCents("104").has_value());
    EXPECT_FALSE(parsePriceCents("104.5").has_value());     // must be 2 decimals
    EXPECT_FALSE(parsePriceCents("104.530").has_value());
    EXPECT_FALSE(parsePriceCents("104.").has_value());
    EXPECT_FALSE(parsePriceCents(".53").has_value());
    EXPECT_FALSE(parsePriceCents("10,53").has_value());
    EXPECT_FALSE(parsePriceCents("10.5a").has_value());
    EXPECT_FALSE(parsePriceCents("-1.00").has_value());     // decide: reject negatives
    EXPECT_FALSE(parsePriceCents(" 1.00 ").has_value());    // tokenizer trims; keep strict here
}