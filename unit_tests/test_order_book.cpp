// unit_tests/test_order_book.cpp

#include <gtest/gtest.h>

#include "book/order_book.hpp"
#include "domain/order.hpp"

namespace {

domain::Order makeOrder(domain::OrderId id,
                        domain::Side side,
                        domain::Price priceCents,
                        int qty = 100,
                        domain::OrderType type = domain::OrderType::Limit,
                        const std::string& symbol = "XYZ",
                        domain::Timestamp ts = 0) {
    domain::Order o;
    o.orderId = id;
    o.side = side;
    o.price = priceCents;
    o.quantity = qty;
    o.orderType = type;
    o.symbol = symbol;
    o.timeStamp = ts;
    return o;
}

} // namespace

TEST(OrderBookTests, EmptyBook_HasZeroCounts_AndContainsIsFalse) {
    OrderBook book;

    EXPECT_EQ(book.liveCount(), 0u);
    EXPECT_EQ(book.buyCount(), 0u);
    EXPECT_EQ(book.sellCount(), 0u);

    EXPECT_FALSE(book.contains(1));
    EXPECT_FALSE(book.contains(123456));
}

TEST(OrderBookTests, AddBuyOrder_IncreasesBuyAndLiveCounts) {
    OrderBook book;

    auto buy = makeOrder(1, domain::Side::Buy, /*price*/ 10453, /*qty*/ 100);

    EXPECT_TRUE(book.add(buy));

    EXPECT_TRUE(book.contains(1));
    EXPECT_EQ(book.liveCount(), 1u);
    EXPECT_EQ(book.buyCount(), 1u);
    EXPECT_EQ(book.sellCount(), 0u);
}

TEST(OrderBookTests, AddSellOrder_IncreasesSellAndLiveCounts) {
    OrderBook book;

    auto sell = makeOrder(2, domain::Side::Sell, /*price*/ 10453, /*qty*/ 100);

    EXPECT_TRUE(book.add(sell));

    EXPECT_TRUE(book.contains(2));
    EXPECT_EQ(book.liveCount(), 1u);
    EXPECT_EQ(book.buyCount(), 0u);
    EXPECT_EQ(book.sellCount(), 1u);
}

TEST(OrderBookTests, DuplicateOrderId_IsRejected_AndCountsDoNotChange) {
    OrderBook book;

    auto buy1 = makeOrder(7, domain::Side::Buy, 10000, 100);
    auto sellDupSameId = makeOrder(7, domain::Side::Sell, 11000, 200);

    EXPECT_TRUE(book.add(buy1));

    const auto liveBefore = book.liveCount();
    const auto buyBefore  = book.buyCount();
    const auto sellBefore = book.sellCount();

    EXPECT_FALSE(book.add(sellDupSameId));

    EXPECT_EQ(book.liveCount(), liveBefore);
    EXPECT_EQ(book.buyCount(), buyBefore);
    EXPECT_EQ(book.sellCount(), sellBefore);
    EXPECT_TRUE(book.contains(7));
}

TEST(OrderBookTests, TwoBuyOrders_SamePriceLevel_CountsTwo) {
    OrderBook book;

    auto o1 = makeOrder(1, domain::Side::Buy, 10000, 100);
    auto o2 = makeOrder(2, domain::Side::Buy, 10000, 150);

    EXPECT_TRUE(book.add(o1));
    EXPECT_TRUE(book.add(o2));

    EXPECT_EQ(book.liveCount(), 2u);
    EXPECT_EQ(book.buyCount(), 2u);
    EXPECT_EQ(book.sellCount(), 0u);
}

TEST(OrderBookTests, TwoBuyOrders_DifferentPriceLevels_CountsTwo) {
    OrderBook book;

    auto o1 = makeOrder(1, domain::Side::Buy, 10000, 100);
    auto o2 = makeOrder(2, domain::Side::Buy, 10100, 100);

    EXPECT_TRUE(book.add(o1));
    EXPECT_TRUE(book.add(o2));

    EXPECT_EQ(book.liveCount(), 2u);
    EXPECT_EQ(book.buyCount(), 2u);
    EXPECT_EQ(book.sellCount(), 0u);
}

TEST(OrderBookTests, MixedSides_CountsSplitCorrectly) {
    OrderBook book;

    auto b1 = makeOrder(1, domain::Side::Buy, 10000, 10);
    auto b2 = makeOrder(2, domain::Side::Buy,  9900, 20);
    auto s1 = makeOrder(3, domain::Side::Sell, 10100, 30);
    auto s2 = makeOrder(4, domain::Side::Sell, 10200, 40);

    EXPECT_TRUE(book.add(b1));
    EXPECT_TRUE(book.add(b2));
    EXPECT_TRUE(book.add(s1));
    EXPECT_TRUE(book.add(s2));

    EXPECT_EQ(book.liveCount(), 4u);
    EXPECT_EQ(book.buyCount(), 2u);
    EXPECT_EQ(book.sellCount(), 2u);

    EXPECT_TRUE(book.contains(1));
    EXPECT_TRUE(book.contains(2));
    EXPECT_TRUE(book.contains(3));
    EXPECT_TRUE(book.contains(4));
}