// unit_tests/engine/test_match.cpp

#include <gtest/gtest.h>

#include "book/order_book.hpp"
#include "engine/match.hpp"
#include "domain/order.hpp"

#include <optional>
#include <string>
#include <vector>

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

// ------------------------- execute() tests -------------------------

TEST(MatchExecuteTests, NoLiquidityOnOneSide_NoEvents) {
    OrderBook book;
    book.add(makeOrder(1, domain::Side::Buy, 10000, 100));

    MatchHandler handler(book);
    MatchRequest req{0, std::nullopt};

    auto resp = handler.execute(req);
    EXPECT_TRUE(resp.events.empty());
}

TEST(MatchExecuteTests, NoPriceCross_NoEvents) {
    OrderBook book;
    book.add(makeOrder(1, domain::Side::Buy, 10000, 100));
    book.add(makeOrder(2, domain::Side::Sell, 11000, 100));

    MatchHandler handler(book);
    MatchRequest req{0, std::nullopt};

    auto resp = handler.execute(req);
    EXPECT_TRUE(resp.events.empty());

    // book unchanged
    EXPECT_TRUE(book.isLive(1));
    EXPECT_TRUE(book.isLive(2));
    EXPECT_EQ(book.liveCount(), 2u);
}

TEST(MatchExecuteTests, SimpleFullMatch_GeneratesSingleEvent_AndRemovesBothOrders) {
    OrderBook book;
    book.add(makeOrder(1, domain::Side::Buy, 10000, 100));
    book.add(makeOrder(2, domain::Side::Sell, 10000, 100));

    MatchHandler handler(book);
    MatchRequest req{0, std::nullopt};

    auto resp = handler.execute(req);

    ASSERT_EQ(resp.events.size(), 1u);
    const auto& e = resp.events[0];

    EXPECT_EQ(e.symbol, "XYZ");
    EXPECT_EQ(e.buyOrderId, 1);
    EXPECT_EQ(e.sellOrderId, 2);
    EXPECT_EQ(e.quantity, 100);
    EXPECT_EQ(e.executionPrice, 10000);

    EXPECT_FALSE(book.isLive(1));
    EXPECT_FALSE(book.isLive(2));
    EXPECT_EQ(book.liveCount(), 0u);
}

TEST(MatchExecuteTests, PartialFill_BuyLarger_RemovesSellAndReducesBuy) {
    OrderBook book;
    book.add(makeOrder(1, domain::Side::Buy, 10000, 120));
    book.add(makeOrder(2, domain::Side::Sell, 10000, 50));

    MatchHandler handler(book);
    MatchRequest req{0, std::nullopt};

    auto resp = handler.execute(req);

    ASSERT_EQ(resp.events.size(), 1u);
    EXPECT_EQ(resp.events[0].quantity, 50);

    // sell fully gone
    EXPECT_FALSE(book.isLive(2));
    // buy remains with 70
    EXPECT_TRUE(book.isLive(1));
    auto* pBuy = book.getById(1);
    ASSERT_NE(pBuy, nullptr);
    EXPECT_EQ(pBuy->quantity, 70);
}

TEST(MatchExecuteTests, FIFOWithinSamePriceLevel_IsRespected) {
    OrderBook book;

    // Two buys same price, FIFO should hit id=1 then id=2
    book.add(makeOrder(1, domain::Side::Buy, 10000, 30, domain::OrderType::Limit, "XYZ", 1));
    book.add(makeOrder(2, domain::Side::Buy, 10000, 30, domain::OrderType::Limit, "XYZ", 2));

    // One sell that matches 40 total => first buy fully (30) and second partially (10)
    book.add(makeOrder(10, domain::Side::Sell, 10000, 40, domain::OrderType::Limit, "XYZ", 3));

    MatchHandler handler(book);
    MatchRequest req{0, std::nullopt};
    auto resp = handler.execute(req);

    ASSERT_EQ(resp.events.size(), 2u);

    // event 0 should be buy id=1
    EXPECT_EQ(resp.events[0].buyOrderId, 1);
    EXPECT_EQ(resp.events[0].sellOrderId, 10);
    EXPECT_EQ(resp.events[0].quantity, 30);

    // event 1 should be buy id=2
    EXPECT_EQ(resp.events[1].buyOrderId, 2);
    EXPECT_EQ(resp.events[1].sellOrderId, 10);
    EXPECT_EQ(resp.events[1].quantity, 10);

    // After matching:
    EXPECT_FALSE(book.isLive(1)); // fully consumed
    EXPECT_TRUE(book.isLive(2));  // partially left
    EXPECT_FALSE(book.isLive(10)); // sell fully consumed

    auto* p2 = book.getById(2);
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(p2->quantity, 20); // 30 - 10
}

TEST(MatchExecuteTests, SymbolFilter_MatchesOnlyThatSymbol_AndLeavesOthersUntouched) {
    OrderBook book;

    // XYZ crosses
    book.add(makeOrder(1, domain::Side::Buy, 10000, 50, domain::OrderType::Limit, "XYZ"));
    book.add(makeOrder(2, domain::Side::Sell, 10000, 50, domain::OrderType::Limit, "XYZ"));

    // ABC also crosses, but should remain after symbol-filter match
    book.add(makeOrder(3, domain::Side::Buy, 20000, 10, domain::OrderType::Limit, "ABC"));
    book.add(makeOrder(4, domain::Side::Sell, 20000, 10, domain::OrderType::Limit, "ABC"));

    MatchHandler handler(book);
    MatchRequest req{0, std::optional<std::string>{"XYZ"}};

    auto resp = handler.execute(req);

    ASSERT_EQ(resp.events.size(), 1u);
    EXPECT_EQ(resp.events[0].symbol, "XYZ");
    EXPECT_FALSE(book.isLive(1));
    EXPECT_FALSE(book.isLive(2));

    // ABC should still be there
    EXPECT_TRUE(book.isLive(3));
    EXPECT_TRUE(book.isLive(4));
}

TEST(MatchExecuteTests, SymbolFilter_NoCrossForThatSymbol_NoEventsEvenIfOtherSymbolsCross) {
    OrderBook book;

    // XYZ no-cross
    book.add(makeOrder(1, domain::Side::Buy, 10000, 10, domain::OrderType::Limit, "XYZ"));
    book.add(makeOrder(2, domain::Side::Sell, 11000, 10, domain::OrderType::Limit, "XYZ"));

    // ABC crosses, but should NOT be matched if request is XYZ
    book.add(makeOrder(3, domain::Side::Buy, 20000, 10, domain::OrderType::Limit, "ABC"));
    book.add(makeOrder(4, domain::Side::Sell, 20000, 10, domain::OrderType::Limit, "ABC"));

    MatchHandler handler(book);
    MatchRequest req{0, std::optional<std::string>{"XYZ"}};

    auto resp = handler.execute(req);

    EXPECT_TRUE(resp.events.empty());

    // everything remains
    EXPECT_TRUE(book.isLive(1));
    EXPECT_TRUE(book.isLive(2));
    EXPECT_TRUE(book.isLive(3));
    EXPECT_TRUE(book.isLive(4));
}

// ------------------------- format() tests -------------------------

TEST(MatchFormatTests, SingleEvent_FormatsExactly) {
    MatchResponse resp;

    TradeEvent e;
    e.symbol = "XYZ";
    e.buyOrderId = 11;
    e.sellOrderId = 110;
    e.buyOrderType = domain::OrderType::Limit;
    e.sellOrderType = domain::OrderType::IOC;
    e.quantity = 100;
    e.executionPrice = 6090;

    resp.events.push_back(e);

    auto out = MatchHandler::format(resp);
    ASSERT_EQ(out.size(), 1u);

    // Note: price is cents (6090), because current format prints raw int.
    EXPECT_EQ(out[0], "XYZ|11,L,100,6090|6090,100,I,110");
}

TEST(MatchFormatTests, MultipleEvents_PreserveOrder) {
    MatchResponse resp;

    resp.events.push_back(TradeEvent{
        "ALN",
        1, 10,
        domain::OrderType::Limit, domain::OrderType::Limit,
        100, 6090
    });

    resp.events.push_back(TradeEvent{
        "XYZ",
        11, 110,
        domain::OrderType::Limit, domain::OrderType::Limit,
        100, 6090
    });

    auto out = MatchHandler::format(resp);
    ASSERT_EQ(out.size(), 2u);

    EXPECT_EQ(out[0], "ALN|1,L,100,6090|6090,100,L,10");
    EXPECT_EQ(out[1], "XYZ|11,L,100,6090|6090,100,L,110");
}