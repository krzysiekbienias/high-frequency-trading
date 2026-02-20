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
    o.orderId   = id;
    o.side      = side;
    o.price     = priceCents;
    o.quantity  = qty;
    o.orderType = type;
    o.symbol    = symbol;
    o.timeStamp = ts;
    return o;
}

} // namespace



TEST(OrderBookTopTests, HasBuyHasSell_EmptyBook_False) {
    OrderBook book;

    EXPECT_FALSE(book.hasBuy());
    EXPECT_FALSE(book.hasSell());
}

TEST(OrderBookTopTests, HasBuyTrue_WhenAnyBuyExists) {
    OrderBook book;

    EXPECT_TRUE(book.add(makeOrder(1, domain::Side::Buy, 10000, 10)));
    EXPECT_TRUE(book.hasBuy());
    EXPECT_FALSE(book.hasSell());
}

TEST(OrderBookTopTests, HasSellTrue_WhenAnySellExists) {
    OrderBook book;

    EXPECT_TRUE(book.add(makeOrder(2, domain::Side::Sell, 10100, 10)));
    EXPECT_FALSE(book.hasBuy());
    EXPECT_TRUE(book.hasSell());
}



TEST(OrderBookTopTests, BestBid_ReturnsHighestBuyPrice) {
    OrderBook book;

    book.add(makeOrder(1, domain::Side::Buy, 10000, 10));
    book.add(makeOrder(2, domain::Side::Buy, 10100, 10));
    book.add(makeOrder(3, domain::Side::Buy,  9900, 10));

    auto bid = book.bestBidPrice();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(*bid, 10100);
}

TEST(OrderBookTopTests, BestAsk_ReturnsLowestSellPrice) {
    OrderBook book;

    book.add(makeOrder(1, domain::Side::Sell, 10500, 10));
    book.add(makeOrder(2, domain::Side::Sell, 10450, 10));
    book.add(makeOrder(3, domain::Side::Sell, 10600, 10));

    auto ask = book.bestAskPrice();
    ASSERT_TRUE(ask.has_value());
    EXPECT_EQ(*ask, 10450);
}


TEST(OrderBookTopTests, BestBidOrder_ReturnsFrontOrderAtBestBidPrice_FIFO) {
    OrderBook book;

    // Best bid level: 10100
    book.add(makeOrder(1, domain::Side::Buy, 10100, 10, domain::OrderType::Limit, "XYZ", 1));
    book.add(makeOrder(2, domain::Side::Buy, 10100, 20, domain::OrderType::Limit, "XYZ", 2));

    // Worse bid level: 10000
    book.add(makeOrder(3, domain::Side::Buy, 10000, 30, domain::OrderType::Limit, "XYZ", 3));

    auto* p = book.bestBidOrder();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->orderId, 1);
    EXPECT_EQ(p->price, 10100);
    EXPECT_EQ(p->quantity, 10);
}

TEST(OrderBookTopTests, BestAskOrder_ReturnsFrontOrderAtBestAskPrice_FIFO) {
    OrderBook book;

    // Best ask level: 10400
    book.add(makeOrder(10, domain::Side::Sell, 10400, 5, domain::OrderType::Limit, "XYZ", 10));
    book.add(makeOrder(11, domain::Side::Sell, 10400, 7, domain::OrderType::Limit, "XYZ", 11));

    // Worse ask level: 10500
    book.add(makeOrder(12, domain::Side::Sell, 10500, 9, domain::OrderType::Limit, "XYZ", 12));

    auto* p = book.bestAskOrder();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->orderId, 10);
    EXPECT_EQ(p->price, 10400);
    EXPECT_EQ(p->quantity, 5);
}

TEST(OrderBookTopTests, BestBidOrder_EmptySide_ReturnsNullptr) {
    OrderBook book;

    EXPECT_EQ(book.bestBidOrder(), nullptr);
    EXPECT_EQ(book.bestAskOrder(), nullptr);

    book.add(makeOrder(1, domain::Side::Buy, 10000, 10));
    EXPECT_NE(book.bestBidOrder(), nullptr);
    EXPECT_EQ(book.bestAskOrder(), nullptr);
}



TEST(OrderBookTests, EmptyBook_HasZeroCounts_AndIsLiveIsFalse) {
    OrderBook book;

    EXPECT_EQ(book.liveCount(), 0u);
    EXPECT_EQ(book.buyCount(), 0u);
    EXPECT_EQ(book.sellCount(), 0u);

    EXPECT_FALSE(book.isLive(1));
    EXPECT_FALSE(book.isLive(123456));
}

TEST(OrderBookTests, AddBuyOrder_IncreasesBuyAndLiveCounts) {
    OrderBook book;

    auto buy = makeOrder(1, domain::Side::Buy, 10453, 100);

    EXPECT_TRUE(book.add(buy));

    EXPECT_TRUE(book.isLive(1));
    EXPECT_EQ(book.liveCount(), 1u);
    EXPECT_EQ(book.buyCount(), 1u);
    EXPECT_EQ(book.sellCount(), 0u);
}

TEST(OrderBookTests, AddSellOrder_IncreasesSellAndLiveCounts) {
    OrderBook book;

    auto sell = makeOrder(2, domain::Side::Sell, 10453, 100);

    EXPECT_TRUE(book.add(sell));

    EXPECT_TRUE(book.isLive(2));
    EXPECT_EQ(book.liveCount(), 1u);
    EXPECT_EQ(book.buyCount(), 0u);
    EXPECT_EQ(book.sellCount(), 1u);
}

TEST(OrderBookTests, DuplicateOrderId_IsRejected_AndCountsDoNotChange) {
    OrderBook book;

    auto buy1          = makeOrder(7, domain::Side::Buy,  10000, 100);
    auto sellDupSameId = makeOrder(7, domain::Side::Sell, 11000, 200);

    EXPECT_TRUE(book.add(buy1));

    const auto liveBefore = book.liveCount();
    const auto buyBefore  = book.buyCount();
    const auto sellBefore = book.sellCount();

    EXPECT_FALSE(book.add(sellDupSameId));

    EXPECT_EQ(book.liveCount(), liveBefore);
    EXPECT_EQ(book.buyCount(), buyBefore);
    EXPECT_EQ(book.sellCount(), sellBefore);
    EXPECT_TRUE(book.isLive(7));
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

    auto b1 = makeOrder(1, domain::Side::Buy,  10000, 10);
    auto b2 = makeOrder(2, domain::Side::Buy,   9900, 20);
    auto s1 = makeOrder(3, domain::Side::Sell, 10100, 30);
    auto s2 = makeOrder(4, domain::Side::Sell, 10200, 40);

    EXPECT_TRUE(book.add(b1));
    EXPECT_TRUE(book.add(b2));
    EXPECT_TRUE(book.add(s1));
    EXPECT_TRUE(book.add(s2));

    EXPECT_EQ(book.liveCount(), 4u);
    EXPECT_EQ(book.buyCount(), 2u);
    EXPECT_EQ(book.sellCount(), 2u);

    EXPECT_TRUE(book.isLive(1));
    EXPECT_TRUE(book.isLive(2));
    EXPECT_TRUE(book.isLive(3));
    EXPECT_TRUE(book.isLive(4));
}

// -------------------- New tests for getById + erase --------------------

TEST(OrderBookLookupEraseTests, GetById_ReturnsNullptr_WhenNotFound) {
    OrderBook book;

    EXPECT_EQ(book.getById(1), nullptr);
    EXPECT_EQ(book.getById(999), nullptr);
}

TEST(OrderBookLookupEraseTests, GetById_FindsBuyOrder_AndAllowsInPlaceUpdate) {
    OrderBook book;

    EXPECT_TRUE(book.add(makeOrder(10, domain::Side::Buy, 10000, 100,
                                   domain::OrderType::Limit, "XYZ", 1)));

    auto* p = book.getById(10);
    ASSERT_NE(p, nullptr);

    EXPECT_EQ(p->orderId, 10);
    EXPECT_EQ(p->symbol, "XYZ");
    EXPECT_EQ(p->price, 10000);
    EXPECT_EQ(p->quantity, 100);

    // in-place modification through pointer
    p->quantity = 60;

    auto* p2 = book.getById(10);
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(p2->quantity, 60);
}

TEST(OrderBookLookupEraseTests, GetById_FindsSellOrder) {
    OrderBook book;

    EXPECT_TRUE(book.add(makeOrder(11, domain::Side::Sell, 10100, 50,
                                   domain::OrderType::Limit, "XYZ", 1)));

    auto* p = book.getById(11);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->side, domain::Side::Sell);
    EXPECT_EQ(p->price, 10100);
    EXPECT_EQ(p->quantity, 50);
}

TEST(OrderBookLookupEraseTests, Erase_ReturnsFalse_WhenNotFound) {
    OrderBook book;

    EXPECT_FALSE(book.erase(123));
    EXPECT_EQ(book.liveCount(), 0u);
    EXPECT_EQ(book.buyCount(), 0u);
    EXPECT_EQ(book.sellCount(), 0u);
}

TEST(OrderBookLookupEraseTests, Erase_RemovesBuyOrder_AndUpdatesCountsAndLiveIds) {
    OrderBook book;

    EXPECT_TRUE(book.add(makeOrder(1, domain::Side::Buy, 10000, 100,
                                   domain::OrderType::Limit, "XYZ", 1)));
    EXPECT_TRUE(book.add(makeOrder(2, domain::Side::Buy, 10000, 100,
                                   domain::OrderType::Limit, "XYZ", 1)));

    EXPECT_EQ(book.liveCount(), 2u);
    EXPECT_EQ(book.buyCount(), 2u);

    EXPECT_TRUE(book.erase(1));

    EXPECT_EQ(book.getById(1), nullptr);
    EXPECT_NE(book.getById(2), nullptr);

    EXPECT_EQ(book.liveCount(), 1u);
    EXPECT_EQ(book.buyCount(), 1u);
    EXPECT_EQ(book.sellCount(), 0u);

    EXPECT_FALSE(book.isLive(1));
    EXPECT_TRUE(book.isLive(2));
}

TEST(OrderBookLookupEraseTests, Erase_RemovesSellOrder_AndUpdatesCounts) {
    OrderBook book;

    EXPECT_TRUE(book.add(makeOrder(3, domain::Side::Sell, 10100, 10,
                                   domain::OrderType::Limit, "XYZ", 1)));
    EXPECT_EQ(book.liveCount(), 1u);
    EXPECT_EQ(book.sellCount(), 1u);

    EXPECT_TRUE(book.erase(3));

    EXPECT_EQ(book.getById(3), nullptr);
    EXPECT_EQ(book.liveCount(), 0u);
    EXPECT_EQ(book.sellCount(), 0u);
    EXPECT_FALSE(book.isLive(3));
}

TEST(OrderBookLookupEraseTests, Erase_RemovesPriceLevel_WhenQueueBecomesEmpty) {
    OrderBook book;

    EXPECT_TRUE(book.add(makeOrder(7, domain::Side::Buy, 12345, 10,
                                   domain::OrderType::Limit, "XYZ", 1)));
    EXPECT_EQ(book.buyCount(), 1u);

    EXPECT_TRUE(book.erase(7));
    EXPECT_EQ(book.buyCount(), 0u);
    EXPECT_EQ(book.getById(7), nullptr);
    EXPECT_EQ(book.liveCount(), 0u);
}

TEST(OrderBookLookupEraseTests, Erase_DoesNotAffectOtherSide) {
    OrderBook book;

    EXPECT_TRUE(book.add(makeOrder(1, domain::Side::Buy, 10000, 100,
                                   domain::OrderType::Limit, "XYZ", 1)));
    EXPECT_TRUE(book.add(makeOrder(2, domain::Side::Sell, 10100, 100,
                                   domain::OrderType::Limit, "XYZ", 1)));

    EXPECT_EQ(book.liveCount(), 2u);
    EXPECT_EQ(book.buyCount(), 1u);
    EXPECT_EQ(book.sellCount(), 1u);

    EXPECT_TRUE(book.erase(1));

    EXPECT_EQ(book.liveCount(), 1u);
    EXPECT_EQ(book.buyCount(), 0u);
    EXPECT_EQ(book.sellCount(), 1u);

    EXPECT_EQ(book.getById(1), nullptr);
    EXPECT_NE(book.getById(2), nullptr);
    EXPECT_FALSE(book.isLive(1));
    EXPECT_TRUE(book.isLive(2));
}