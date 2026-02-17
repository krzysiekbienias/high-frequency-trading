#include <gtest/gtest.h>

#include "book/order_book.hpp"
#include "engine/new.hpp"
#include "domain/order.hpp"

namespace {

domain::Order makeOrder(domain::OrderId id,
                        domain::Timestamp ts,
                        const std::string& symbol,
                        domain::OrderType type,
                        domain::Side side,
                        domain::Price priceCents,
                        int qty) {
    domain::Order o;
    o.orderId = id;
    o.timeStamp = ts;
    o.symbol = symbol;
    o.orderType = type;
    o.side = side;
    o.price = priceCents;
    o.quantity = qty;
    return o;
}

} // namespace

TEST(NewCommandTests, AcceptsValidLimitOrder_AndAddsToBook) {
    OrderBook book;
    NewCommandHandler handler(book);

    auto o = makeOrder(2, 2, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 10453, 100);
    auto r = handler.execute(o);

    EXPECT_TRUE(r.accepted);
    EXPECT_EQ(book.liveCount(), 1u);
    EXPECT_TRUE(book.contains(2));
}

TEST(NewCommandTests, RejectsDuplicateOrderId) {
    OrderBook book;
    NewCommandHandler handler(book);

    auto o1 = makeOrder(2, 2, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 10453, 100);
    auto o2 = makeOrder(2, 3, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 10453, 100);

    EXPECT_TRUE(handler.execute(o1).accepted);
    auto r2 = handler.execute(o2);

    EXPECT_FALSE(r2.accepted);
    EXPECT_EQ(book.liveCount(), 1u);
    EXPECT_EQ(book.buyCount(), 1u);
}

TEST(NewCommandTests, RejectsMarketWithNonZeroPrice) {
    OrderBook book;
    NewCommandHandler handler(book);

    auto o = makeOrder(10, 1, "XYZ", domain::OrderType::Market, domain::Side::Buy, 1, 100);
    auto r = handler.execute(o);

    EXPECT_FALSE(r.accepted);
    EXPECT_EQ(book.liveCount(), 0u);
}

TEST(NewCommandTests, AcceptsMarketWithZeroPrice) {
    OrderBook book;
    NewCommandHandler handler(book);

    auto o = makeOrder(11, 1, "XYZ", domain::OrderType::Market, domain::Side::Buy, 0, 100);
    auto r = handler.execute(o);

    EXPECT_TRUE(r.accepted);
    EXPECT_EQ(book.liveCount(), 1u);
}

TEST(NewCommandTests, RejectsLimitWithZeroPrice) {
    OrderBook book;
    NewCommandHandler handler(book);

    auto o = makeOrder(12, 1, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 0, 100);
    auto r = handler.execute(o);

    EXPECT_FALSE(r.accepted);
    EXPECT_EQ(book.liveCount(), 0u);
}

TEST(NewCommandTests, RejectsNonAlphaSymbol) {
    OrderBook book;
    NewCommandHandler handler(book);

    auto o = makeOrder(13, 1, "X1Z", domain::OrderType::Limit, domain::Side::Buy, 10000, 100);
    auto r = handler.execute(o);

    EXPECT_FALSE(r.accepted);
    EXPECT_EQ(book.liveCount(), 0u);
}

TEST(NewCommandTests, RejectsNonPositiveQuantity) {
    OrderBook book;
    NewCommandHandler handler(book);

    auto o = makeOrder(14, 1, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 10000, 0);
    auto r = handler.execute(o);

    EXPECT_FALSE(r.accepted);
    EXPECT_EQ(book.liveCount(), 0u);
}

TEST(NewCommandTests, FormatMatchesSpec) {
    NewCommandResult ok;
    ok.orderId = 2;
    ok.accepted = true;

    EXPECT_EQ(NewCommandHandler::format(ok), "2 - Accept");

    NewCommandResult bad;
    bad.orderId = 2;
    bad.accepted = false;

    EXPECT_EQ(NewCommandHandler::format(bad), "2 - Reject - 303 - Invalid order details");
}