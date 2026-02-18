#include <gtest/gtest.h>

#include "engine/cancel.hpp"
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

CancelRequest makeCancel(domain::OrderId id, domain::Timestamp ts) {
    CancelRequest r;
    r.orderId = id;
    r.timeStamp = ts;
    return r;
}

} // namespace

TEST(CancelTests, Reject101_WhenOrderIdInvalid) {
    OrderBook book;
    CancelHandler cancel(book);

    auto res = cancel.execute(makeCancel(0, 1));

    EXPECT_FALSE(res.accepted);
    EXPECT_EQ(res.rejectCode, 101);
}

TEST(CancelTests, Reject101_WhenTimestampNegative) {
    OrderBook book;
    CancelHandler cancel(book);

    auto res = cancel.execute(makeCancel(1, -1));

    EXPECT_FALSE(res.accepted);
    EXPECT_EQ(res.rejectCode, 101);
}

TEST(CancelTests, Reject404_WhenOrderDoesNotExist) {
    OrderBook book;
    CancelHandler cancel(book);

    auto res = cancel.execute(makeCancel(123, 1));

    EXPECT_FALSE(res.accepted);
    EXPECT_EQ(res.rejectCode, 404);
    EXPECT_EQ(res.rejectMessage, "Order does not exist");
}

TEST(CancelTests, Accept_RemovesOrderFromBook) {
    OrderBook book;
    CancelHandler cancel(book);

    EXPECT_TRUE(book.add(makeOrder(1, domain::Side::Buy, 10000, 100)));
    EXPECT_TRUE(book.isLive(1));
    EXPECT_EQ(book.liveCount(), 1u);
    EXPECT_EQ(book.buyCount(), 1u);

    auto res = cancel.execute(makeCancel(1, 10));

    EXPECT_TRUE(res.accepted);
    EXPECT_FALSE(book.isLive(1));
    EXPECT_EQ(book.liveCount(), 0u);
    EXPECT_EQ(book.buyCount(), 0u);
    EXPECT_EQ(book.sellCount(), 0u);
}

TEST(CancelTests, Accept_WorksForSellSideToo) {
    OrderBook book;
    CancelHandler cancel(book);

    EXPECT_TRUE(book.add(makeOrder(2, domain::Side::Sell, 10100, 50)));
    EXPECT_TRUE(book.isLive(2));
    EXPECT_EQ(book.sellCount(), 1u);

    auto res = cancel.execute(makeCancel(2, 20));

    EXPECT_TRUE(res.accepted);
    EXPECT_FALSE(book.isLive(2));
    EXPECT_EQ(book.liveCount(), 0u);
    EXPECT_EQ(book.sellCount(), 0u);
}

TEST(CancelTests, DoubleCancel_SecondTimeReject404) {
    OrderBook book;
    CancelHandler cancel(book);

    EXPECT_TRUE(book.add(makeOrder(7, domain::Side::Buy, 10000, 100)));
    EXPECT_TRUE(book.isLive(7));

    auto res1 = cancel.execute(makeCancel(7, 1));
    EXPECT_TRUE(res1.accepted);
    EXPECT_FALSE(book.isLive(7));

    auto res2 = cancel.execute(makeCancel(7, 2));
    EXPECT_FALSE(res2.accepted);
    EXPECT_EQ(res2.rejectCode, 404);
}

TEST(CancelTests, FormatMatchesSpec) {
    CancelResponse ok;
    ok.orderId = 1;
    ok.accepted = true;
    EXPECT_EQ(CancelHandler::format(ok), "1 - CancelAccept");

    CancelResponse bad;
    bad.orderId = 2;
    bad.accepted = false;
    bad.rejectCode = 404;
    bad.rejectMessage = "Order does not exist";
    EXPECT_EQ(CancelHandler::format(bad), "2 - CancelReject - 404 - Order does not exist");
}