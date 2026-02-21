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


// --- consumeBestBid/consumeBestAsk tests ---

TEST(OrderBookConsumeTests, ConsumeBestBid_ReducesQuantity_WhenPartialFill) {
    OrderBook book;

    // Best bid: 10100, qty=100
    book.add(makeOrder(1, domain::Side::Buy, 10100, 100));

    ASSERT_TRUE(book.bestBidPrice().has_value());
    EXPECT_EQ(*book.bestBidPrice(), 10100);

    auto* bid = book.bestBidOrder();
    ASSERT_NE(bid, nullptr);
    EXPECT_EQ(bid->quantity, 100);

    book.consumeBestBid(40);

    auto* bid2 = book.bestBidOrder();
    ASSERT_NE(bid2, nullptr);
    EXPECT_EQ(bid2->orderId, 1);
    EXPECT_EQ(bid2->quantity, 60);

    EXPECT_TRUE(book.isLive(1));
    EXPECT_EQ(book.buyCount(), 1u);
    EXPECT_EQ(book.liveCount(), 1u);
}

TEST(OrderBookConsumeTests, ConsumeBestAsk_ReducesQuantity_WhenPartialFill) {
    OrderBook book;

    // Best ask: 10000, qty=50
    book.add(makeOrder(2, domain::Side::Sell, 10000, 50));

    ASSERT_TRUE(book.bestAskPrice().has_value());
    EXPECT_EQ(*book.bestAskPrice(), 10000);

    auto* ask = book.bestAskOrder();
    ASSERT_NE(ask, nullptr);
    EXPECT_EQ(ask->quantity, 50);

    book.consumeBestAsk(20);

    auto* ask2 = book.bestAskOrder();
    ASSERT_NE(ask2, nullptr);
    EXPECT_EQ(ask2->orderId, 2);
    EXPECT_EQ(ask2->quantity, 30);

    EXPECT_TRUE(book.isLive(2));
    EXPECT_EQ(book.sellCount(), 1u);
    EXPECT_EQ(book.liveCount(), 1u);
}

TEST(OrderBookConsumeTests, ConsumeBestBid_RemovesOrderAndLiveId_WhenFullyFilled) {
    OrderBook book;

    book.add(makeOrder(1, domain::Side::Buy, 10100, 100));
    EXPECT_TRUE(book.isLive(1));
    EXPECT_EQ(book.buyCount(), 1u);
    EXPECT_EQ(book.liveCount(), 1u);

    book.consumeBestBid(100);

    // Order removed
    EXPECT_EQ(book.bestBidOrder(), nullptr);
    EXPECT_FALSE(book.isLive(1));

    EXPECT_EQ(book.buyCount(), 0u);
    EXPECT_EQ(book.liveCount(), 0u);
    EXPECT_FALSE(book.bestBidPrice().has_value());
}

TEST(OrderBookConsumeTests, ConsumeBestAsk_RemovesOrderAndLiveId_WhenFullyFilled) {
    OrderBook book;

    book.add(makeOrder(2, domain::Side::Sell, 10000, 50));
    EXPECT_TRUE(book.isLive(2));
    EXPECT_EQ(book.sellCount(), 1u);
    EXPECT_EQ(book.liveCount(), 1u);

    book.consumeBestAsk(50);

    EXPECT_EQ(book.bestAskOrder(), nullptr);
    EXPECT_FALSE(book.isLive(2));

    EXPECT_EQ(book.sellCount(), 0u);
    EXPECT_EQ(book.liveCount(), 0u);
    EXPECT_FALSE(book.bestAskPrice().has_value());
}

TEST(OrderBookConsumeTests, ConsumeBestBid_PopsFrontFIFO_WhenFirstOrderFullyFilled) {
    OrderBook book;

    // Two orders at same best price level 10100; FIFO: id=1 then id=2
    book.add(makeOrder(1, domain::Side::Buy, 10100, 10));
    book.add(makeOrder(2, domain::Side::Buy, 10100, 20));

    auto* first = book.bestBidOrder();
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->orderId, 1);

    // Fully fill first
    book.consumeBestBid(10);

    auto* second = book.bestBidOrder();
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->orderId, 2);
    EXPECT_EQ(second->quantity, 20);

    EXPECT_FALSE(book.isLive(1));
    EXPECT_TRUE(book.isLive(2));
    EXPECT_EQ(book.buyCount(), 1u);
    EXPECT_EQ(book.liveCount(), 1u);
}

TEST(OrderBookConsumeTests, ConsumeBestAsk_PopsFrontFIFO_WhenFirstOrderFullyFilled) {
    OrderBook book;

    // Two orders at same best ask 10000; FIFO: id=10 then id=11
    book.add(makeOrder(10, domain::Side::Sell, 10000, 5));
    book.add(makeOrder(11, domain::Side::Sell, 10000, 7));

    auto* first = book.bestAskOrder();
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->orderId, 10);

    book.consumeBestAsk(5);

    auto* second = book.bestAskOrder();
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->orderId, 11);
    EXPECT_EQ(second->quantity, 7);

    EXPECT_FALSE(book.isLive(10));
    EXPECT_TRUE(book.isLive(11));
    EXPECT_EQ(book.sellCount(), 1u);
    EXPECT_EQ(book.liveCount(), 1u);
}

TEST(OrderBookConsumeTests, ConsumeBestBid_RemovesPriceLevel_WhenQueueBecomesEmpty) {
    OrderBook book;

    // Best bid 10100 has one order, worse bid 10000 has one order
    book.add(makeOrder(1, domain::Side::Buy, 10100, 10));
    book.add(makeOrder(2, domain::Side::Buy, 10000, 10));

    ASSERT_TRUE(book.bestBidPrice().has_value());
    EXPECT_EQ(*book.bestBidPrice(), 10100);

    // Consume the only order at 10100 -> should remove that level and move bestBid to 10000
    book.consumeBestBid(10);

    ASSERT_TRUE(book.bestBidPrice().has_value());
    EXPECT_EQ(*book.bestBidPrice(), 10000);

    auto* best = book.bestBidOrder();
    ASSERT_NE(best, nullptr);
    EXPECT_EQ(best->orderId, 2);
}

TEST(OrderBookConsumeTests, ConsumeBestAsk_RemovesPriceLevel_WhenQueueBecomesEmpty) {
    OrderBook book;

    // Best ask 10000 has one order, worse ask 10100 has one order
    book.add(makeOrder(10, domain::Side::Sell, 10000, 5));
    book.add(makeOrder(11, domain::Side::Sell, 10100, 5));

    ASSERT_TRUE(book.bestAskPrice().has_value());
    EXPECT_EQ(*book.bestAskPrice(), 10000);

    book.consumeBestAsk(5);

    ASSERT_TRUE(book.bestAskPrice().has_value());
    EXPECT_EQ(*book.bestAskPrice(), 10100);

    auto* best = book.bestAskOrder();
    ASSERT_NE(best, nullptr);
    EXPECT_EQ(best->orderId, 11);
}



// --- bestBidPrice(symbol) tests ---

TEST(OrderBookBestBidPriceBySymbolTests, EmptyBook_ReturnsNullopt) {
    OrderBook book;
    auto p = book.bestBidPrice("XYZ");
    EXPECT_FALSE(p.has_value());
}

TEST(OrderBookBestBidPriceBySymbolTests, NoMatchingSymbol_ReturnsNullopt) {
    OrderBook book;

    book.add(makeOrder(1, domain::Side::Buy, 10100, 10, domain::OrderType::Limit, "ABC", 1));
    book.add(makeOrder(2, domain::Side::Buy, 10000, 10, domain::OrderType::Limit, "DEF", 2));

    auto p = book.bestBidPrice("XYZ");
    EXPECT_FALSE(p.has_value());
}

TEST(OrderBookBestBidPriceBySymbolTests, ReturnsBestPriceLevelWhereSymbolExists) {
    OrderBook book;

    // Best price level is 10500 but contains only ABC
    book.add(makeOrder(1, domain::Side::Buy, 10500, 10, domain::OrderType::Limit, "ABC", 1));

    // Next level is 10400 and contains XYZ -> expected answer is 10400
    book.add(makeOrder(2, domain::Side::Buy, 10400, 10, domain::OrderType::Limit, "XYZ", 2));

    // Worse level also contains XYZ but should not matter
    book.add(makeOrder(3, domain::Side::Buy, 10300, 10, domain::OrderType::Limit, "XYZ", 3));

    auto p = book.bestBidPrice("XYZ");
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(*p, 10400);
}

TEST(OrderBookBestBidPriceBySymbolTests, WorksWhenSymbolIsNotAtFrontOfDequeInBestLevel) {
    OrderBook book;

    // Same best price level 10100, FIFO queue:
    // First order is ABC, second is XYZ; function should still find XYZ in the same level.
    book.add(makeOrder(1, domain::Side::Buy, 10100, 10, domain::OrderType::Limit, "ABC", 1));
    book.add(makeOrder(2, domain::Side::Buy, 10100, 10, domain::OrderType::Limit, "XYZ", 2));

    auto p = book.bestBidPrice("XYZ");
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(*p, 10100);
}

// --- bestAskPrice(symbol) tests ---

TEST(OrderBookBestAskPriceBySymbolTests, EmptyBook_ReturnsNullopt) {
    OrderBook book;
    auto p = book.bestAskPrice("XYZ");
    EXPECT_FALSE(p.has_value());
}

TEST(OrderBookBestAskPriceBySymbolTests, NoMatchingSymbol_ReturnsNullopt) {
    OrderBook book;

    book.add(makeOrder(1, domain::Side::Sell, 10100, 10, domain::OrderType::Limit, "ABC", 1));
    book.add(makeOrder(2, domain::Side::Sell, 10000, 10, domain::OrderType::Limit, "DEF", 2));

    auto p = book.bestAskPrice("XYZ");
    EXPECT_FALSE(p.has_value());
}

TEST(OrderBookBestAskPriceBySymbolTests, ReturnsBestAskLevelWhereSymbolExists) {
    OrderBook book;

    // Best ask level is 10000 but contains only ABC
    book.add(makeOrder(1, domain::Side::Sell, 10000, 10, domain::OrderType::Limit, "ABC", 1));

    // Next level is 10100 and contains XYZ -> expected answer is 10100
    book.add(makeOrder(2, domain::Side::Sell, 10100, 10, domain::OrderType::Limit, "XYZ", 2));

    // Worse (higher) level also contains XYZ but should not matter
    book.add(makeOrder(3, domain::Side::Sell, 10200, 10, domain::OrderType::Limit, "XYZ", 3));

    auto p = book.bestAskPrice("XYZ");
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(*p, 10100);
}

TEST(OrderBookBestAskPriceBySymbolTests, WorksWhenSymbolIsNotAtFrontOfDequeInBestLevel) {
    OrderBook book;

    // Same best ask price level 10000, FIFO queue:
    // First order is ABC, second is XYZ; function should still find XYZ in the same level.
    book.add(makeOrder(1, domain::Side::Sell, 10000, 10, domain::OrderType::Limit, "ABC", 1));
    book.add(makeOrder(2, domain::Side::Sell, 10000, 10, domain::OrderType::Limit, "XYZ", 2));

    auto p = book.bestAskPrice("XYZ");
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(*p, 10000);



}

// --- bestBidOrder(symbol) tests ---

TEST(OrderBookBestBidOrderBySymbolTests, EmptyBook_ReturnsNullptr) {
    OrderBook book;
    EXPECT_EQ(book.bestBidOrder("XYZ"), nullptr);
}

TEST(OrderBookBestBidOrderBySymbolTests, NoMatchingSymbol_ReturnsNullptr) {
    OrderBook book;

    book.add(makeOrder(1, domain::Side::Buy, 10500, 10, domain::OrderType::Limit, "ABC", 1));
    book.add(makeOrder(2, domain::Side::Buy, 10400, 10, domain::OrderType::Limit, "DEF", 2));

    EXPECT_EQ(book.bestBidOrder("XYZ"), nullptr);
}

TEST(OrderBookBestBidOrderBySymbolTests, ReturnsOrderFromBestPriceLevelThatContainsSymbol) {
    OrderBook book;

    // Best price level (10500) has no XYZ
    book.add(makeOrder(1, domain::Side::Buy, 10500, 10, domain::OrderType::Limit, "ABC", 1));

    // Next level (10400) has XYZ -> should return this one
    book.add(makeOrder(2, domain::Side::Buy, 10400, 20, domain::OrderType::Limit, "XYZ", 2));

    // Worse level also has XYZ but should not be chosen
    book.add(makeOrder(3, domain::Side::Buy, 10300, 30, domain::OrderType::Limit, "XYZ", 3));

    auto* p = book.bestBidOrder("XYZ");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->orderId, 2);
    EXPECT_EQ(p->price, 10400);
    EXPECT_EQ(p->symbol, "XYZ");
}

TEST(OrderBookBestBidOrderBySymbolTests, ReturnsFirstMatchingOrderInDequeFIFOWithinSamePriceLevel) {
    OrderBook book;

    // Same price level 10100, FIFO:
    // id=1 ABC, id=2 XYZ, id=3 XYZ -> bestBidOrder("XYZ") should return id=2
    book.add(makeOrder(1, domain::Side::Buy, 10100, 10, domain::OrderType::Limit, "ABC", 1));
    book.add(makeOrder(2, domain::Side::Buy, 10100, 20, domain::OrderType::Limit, "XYZ", 2));
    book.add(makeOrder(3, domain::Side::Buy, 10100, 30, domain::OrderType::Limit, "XYZ", 3));

    auto* p = book.bestBidOrder("XYZ");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->orderId, 2);
    EXPECT_EQ(p->price, 10100);
}

// --- bestAskOrder(symbol) tests ---

TEST(OrderBookBestAskOrderBySymbolTests, EmptyBook_ReturnsNullptr) {
    OrderBook book;
    EXPECT_EQ(book.bestAskOrder("XYZ"), nullptr);
}

TEST(OrderBookBestAskOrderBySymbolTests, NoMatchingSymbol_ReturnsNullptr) {
    OrderBook book;

    book.add(makeOrder(1, domain::Side::Sell, 10000, 10, domain::OrderType::Limit, "ABC", 1));
    book.add(makeOrder(2, domain::Side::Sell, 10100, 10, domain::OrderType::Limit, "DEF", 2));

    EXPECT_EQ(book.bestAskOrder("XYZ"), nullptr);
}

TEST(OrderBookBestAskOrderBySymbolTests, ReturnsOrderFromBestAskLevelThatContainsSymbol) {
    OrderBook book;

    // Best ask level (10000) has no XYZ
    book.add(makeOrder(1, domain::Side::Sell, 10000, 10, domain::OrderType::Limit, "ABC", 1));

    // Next level (10100) has XYZ -> should return this one
    book.add(makeOrder(2, domain::Side::Sell, 10100, 20, domain::OrderType::Limit, "XYZ", 2));

    // Worse (higher) level also has XYZ but should not be chosen
    book.add(makeOrder(3, domain::Side::Sell, 10200, 30, domain::OrderType::Limit, "XYZ", 3));

    auto* p = book.bestAskOrder("XYZ");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->orderId, 2);
    EXPECT_EQ(p->price, 10100);
    EXPECT_EQ(p->symbol, "XYZ");
}

TEST(OrderBookBestAskOrderBySymbolTests, ReturnsFirstMatchingOrderInDequeFIFOWithinSamePriceLevel) {
    OrderBook book;

    // Same best ask price level 10000, FIFO:
    // id=1 ABC, id=2 XYZ, id=3 XYZ -> bestAskOrder("XYZ") should return id=2
    book.add(makeOrder(1, domain::Side::Sell, 10000, 10, domain::OrderType::Limit, "ABC", 1));
    book.add(makeOrder(2, domain::Side::Sell, 10000, 20, domain::OrderType::Limit, "XYZ", 2));
    book.add(makeOrder(3, domain::Side::Sell, 10000, 30, domain::OrderType::Limit, "XYZ", 3));

    auto* p = book.bestAskOrder("XYZ");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->orderId, 2);
    EXPECT_EQ(p->price, 10000);
}


// --- consumeBestBid(qty, symbol) tests ---

TEST(OrderBookConsumeBestBidBySymbolTests, DoesNothing_WhenBookEmpty) {
    OrderBook book;

    book.consumeBestBid(10, "XYZ");

    EXPECT_EQ(book.liveCount(), 0u);
    EXPECT_EQ(book.buyCount(), 0u);
}

TEST(OrderBookConsumeBestBidBySymbolTests, DoesNothing_WhenNoOrderForSymbolExists) {
    OrderBook book;

    book.add(makeOrder(1, domain::Side::Buy, 10100, 10, domain::OrderType::Limit, "ABC", 1));
    book.add(makeOrder(2, domain::Side::Buy, 10000, 20, domain::OrderType::Limit, "DEF", 2));

    book.consumeBestBid(5, "XYZ"); // symbol not present

    EXPECT_TRUE(book.isLive(1));
    EXPECT_TRUE(book.isLive(2));
    EXPECT_EQ(book.liveCount(), 2u);
    EXPECT_EQ(book.buyCount(), 2u);
}

TEST(OrderBookConsumeBestBidBySymbolTests, ConsumesFromBestPriceLevelThatContainsSymbol) {
    OrderBook book;

    // Best level 10500 has no XYZ
    book.add(makeOrder(1, domain::Side::Buy, 10500, 10, domain::OrderType::Limit, "ABC", 1));

    // Next level 10400 has XYZ -> should be consumed here
    book.add(makeOrder(2, domain::Side::Buy, 10400, 20, domain::OrderType::Limit, "XYZ", 2));

    book.consumeBestBid(7, "XYZ");

    auto* p = book.getById(2);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->price, 10400);
    EXPECT_EQ(p->quantity, 13); // 20 - 7

    // ensure the best level order untouched
    auto* p1 = book.getById(1);
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(p1->quantity, 10);
}

TEST(OrderBookConsumeBestBidBySymbolTests, FullyFillsOrder_RemovesIt_AndKeepsFIFOWithinLevelForSameSymbol) {
    OrderBook book;

    // Same price level 10100. FIFO:
    // id=1 ABC, id=2 XYZ, id=3 XYZ
    book.add(makeOrder(1, domain::Side::Buy, 10100, 10, domain::OrderType::Limit, "ABC", 1));
    book.add(makeOrder(2, domain::Side::Buy, 10100, 10, domain::OrderType::Limit, "XYZ", 2));
    book.add(makeOrder(3, domain::Side::Buy, 10100, 20, domain::OrderType::Limit, "XYZ", 3));

    // Fully fill the first XYZ (id=2)
    book.consumeBestBid(10, "XYZ");

    EXPECT_FALSE(book.isLive(2));
    EXPECT_TRUE(book.isLive(1));
    EXPECT_TRUE(book.isLive(3));

    EXPECT_EQ(book.liveCount(), 2u);
    EXPECT_EQ(book.buyCount(), 2u);

    // Now bestBidOrder("XYZ") should point to id=3 (next FIFO for XYZ)
    auto* next = book.bestBidOrder("XYZ");
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->orderId, 3);
    EXPECT_EQ(next->quantity, 20);
}

// --- consumeBestAsk(qty, symbol) tests ---

TEST(OrderBookConsumeBestAskBySymbolTests, DoesNothing_WhenBookEmpty) {
    OrderBook book;

    book.consumeBestAsk(10, "XYZ");

    EXPECT_EQ(book.liveCount(), 0u);
    EXPECT_EQ(book.sellCount(), 0u);
}

TEST(OrderBookConsumeBestAskBySymbolTests, DoesNothing_WhenNoOrderForSymbolExists) {
    OrderBook book;

    book.add(makeOrder(1, domain::Side::Sell, 10000, 10, domain::OrderType::Limit, "ABC", 1));
    book.add(makeOrder(2, domain::Side::Sell, 10100, 20, domain::OrderType::Limit, "DEF", 2));

    book.consumeBestAsk(5, "XYZ"); // symbol not present

    EXPECT_TRUE(book.isLive(1));
    EXPECT_TRUE(book.isLive(2));
    EXPECT_EQ(book.liveCount(), 2u);
    EXPECT_EQ(book.sellCount(), 2u);
}

TEST(OrderBookConsumeBestAskBySymbolTests, ConsumesFromBestAskLevelThatContainsSymbol) {
    OrderBook book;

    // Best ask level 10000 has no XYZ
    book.add(makeOrder(1, domain::Side::Sell, 10000, 10, domain::OrderType::Limit, "ABC", 1));

    // Next ask level 10100 has XYZ -> should be consumed here
    book.add(makeOrder(2, domain::Side::Sell, 10100, 20, domain::OrderType::Limit, "XYZ", 2));

    book.consumeBestAsk(7, "XYZ");

    auto* p = book.getById(2);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->price, 10100);
    EXPECT_EQ(p->quantity, 13); // 20 - 7

    // ensure best ask level order untouched
    auto* p1 = book.getById(1);
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(p1->quantity, 10);
}

TEST(OrderBookConsumeBestAskBySymbolTests, FullyFillsOrder_RemovesIt_AndKeepsFIFOWithinLevelForSameSymbol) {
    OrderBook book;

    // Same ask price level 10000. FIFO:
    // id=1 ABC, id=2 XYZ, id=3 XYZ
    book.add(makeOrder(1, domain::Side::Sell, 10000, 10, domain::OrderType::Limit, "ABC", 1));
    book.add(makeOrder(2, domain::Side::Sell, 10000, 10, domain::OrderType::Limit, "XYZ", 2));
    book.add(makeOrder(3, domain::Side::Sell, 10000, 20, domain::OrderType::Limit, "XYZ", 3));

    // Fully fill the first XYZ (id=2)
    book.consumeBestAsk(10, "XYZ");

    EXPECT_FALSE(book.isLive(2));
    EXPECT_TRUE(book.isLive(1));
    EXPECT_TRUE(book.isLive(3));

    EXPECT_EQ(book.liveCount(), 2u);
    EXPECT_EQ(book.sellCount(), 2u);

    // Now bestAskOrder("XYZ") should point to id=3 (next FIFO for XYZ)
    auto* next = book.bestAskOrder("XYZ");
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->orderId, 3);
    EXPECT_EQ(next->quantity, 20);
}