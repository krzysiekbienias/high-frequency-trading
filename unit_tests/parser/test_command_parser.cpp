#include <gtest/gtest.h>

#include "parser/commands_parser.hpp"

// Helpers to check variant type
template <typename T>
static bool holds(const ParsedCommand& cmd) {
    return std::holds_alternative<T>(cmd);
}

TEST(CommandParserTests, EmptyLine_ReturnsNullopt) {
    EXPECT_FALSE(parseCommandLine("").has_value());
    EXPECT_FALSE(parseCommandLine("\n").has_value());
    EXPECT_FALSE(parseCommandLine("\r\n").has_value());
}

TEST(CommandParserTests, UnknownCommand_ReturnsNullopt) {
    EXPECT_FALSE(parseCommandLine("Z,1,2,XYZ,L,B,104.53,100").has_value());
    EXPECT_FALSE(parseCommandLine("?,1,2,XYZ,L,B,104.53,100").has_value());
}

TEST(CommandParserTests, NewCommand_ValidLine_ParsesToDomainOrder) {
    auto cmd = parseCommandLine("N,2,00000002,XYZ,L,B,104.53,100");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_TRUE(holds<domain::Order>(*cmd));

    const auto& o = std::get<domain::Order>(*cmd);
    EXPECT_EQ(o.orderId, 2);
    EXPECT_EQ(o.timeStamp, 2);                 // "00000002" -> 2
    EXPECT_EQ(o.symbol, "XYZ");
    EXPECT_EQ(o.orderType, domain::OrderType::Limit);
    EXPECT_EQ(o.side, domain::Side::Buy);
    EXPECT_EQ(o.price, 10453);                 // cents
    EXPECT_EQ(o.quantity, 100);
}

TEST(CommandParserTests, NewCommand_InvalidArity_ReturnsNullopt) {
    EXPECT_FALSE(parseCommandLine("N,2,00000002,XYZ,L,B,104.53").has_value());          // 7 tokens
    EXPECT_FALSE(parseCommandLine("N,2,00000002,XYZ,L,B,104.53,100,EXTRA").has_value());// 9 tokens
}

TEST(CommandParserTests, NewCommand_InvalidField_ReturnsNullopt) {
    EXPECT_FALSE(parseCommandLine("N,0,00000002,XYZ,L,B,104.53,100").has_value());      // orderId must be >0
    EXPECT_FALSE(parseCommandLine("N,2,-1,XYZ,L,B,104.53,100").has_value());            // timestamp >=0
    EXPECT_FALSE(parseCommandLine("N,2,00000002,XYZ,L,B,104.5,100").has_value());       // price format
    EXPECT_FALSE(parseCommandLine("N,2,00000002,XYZ,L,B,104.53,0").has_value());        // qty >0
    EXPECT_FALSE(parseCommandLine("N,2,00000002,XYZ,L,X,104.53,100").has_value());      // side invalid
    EXPECT_FALSE(parseCommandLine("N,2,00000002,XYZ,X,B,104.53,100").has_value());      // orderType invalid
}

TEST(CommandParserTests, AmendCommand_ValidLine_ParsesToAmendRequest) {
    auto cmd = parseCommandLine("A,2,00000003,XYZ,L,B,105.00,150");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_TRUE(holds<AmendRequest>(*cmd));

    const auto& req = std::get<AmendRequest>(*cmd);
    EXPECT_EQ(req.orderId, 2);
    EXPECT_EQ(req.timeStamp, 3);
    EXPECT_EQ(req.symbol, "XYZ");
    EXPECT_EQ(req.orderType, domain::OrderType::Limit);
    EXPECT_EQ(req.side, domain::Side::Buy);
    EXPECT_EQ(req.newPrice, 10500);
    EXPECT_EQ(req.newQuantity, 150);
}

TEST(CommandParserTests, AmendCommand_InvalidArity_ReturnsNullopt) {
    EXPECT_FALSE(parseCommandLine("A,2,3,XYZ,L,B,105.00").has_value());
    EXPECT_FALSE(parseCommandLine("A,2,3,XYZ,L,B,105.00,150,EXTRA").has_value());
}

TEST(CommandParserTests, CancelCommand_ValidLine_ParsesToCancelRequest) {
    auto cmd = parseCommandLine("X,2,00000005");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_TRUE(holds<CancelRequest>(*cmd));

    const auto& req = std::get<CancelRequest>(*cmd);
    EXPECT_EQ(req.orderId, 2);
    EXPECT_EQ(req.timeStamp, 5);
}

TEST(CommandParserTests, CancelCommand_InvalidArity_ReturnsNullopt) {
    EXPECT_FALSE(parseCommandLine("X,2").has_value());
    EXPECT_FALSE(parseCommandLine("X,2,3,EXTRA").has_value());
}

TEST(CommandParserTests, CancelCommand_InvalidField_ReturnsNullopt) {
    EXPECT_FALSE(parseCommandLine("X,0,1").has_value());       // orderId >0
    EXPECT_FALSE(parseCommandLine("X,2,-1").has_value());      // timestamp >=0
    EXPECT_FALSE(parseCommandLine("X,abc,1").has_value());     // invalid int
}