#include <gtest/gtest.h>

#include "engine/dispatcher.hpp"

// Helper factory for domain::Order
static domain::Order makeOrder(domain::OrderId id,
                               domain::Timestamp ts = 1,
                               const std::string& sym = "XYZ",
                               domain::OrderType type = domain::OrderType::Limit,
                               domain::Side side = domain::Side::Buy,
                               domain::Price price = 10453,
                               int qty = 100) {
    domain::Order o{};
    o.orderId = id;
    o.timeStamp = ts;
    o.symbol = sym;
    o.orderType = type;
    o.side = side;
    o.price = price;
    o.quantity = qty;
    return o;
}

TEST(DispatcherTests, DispatchNew_Accept_PrintsAcceptLine) {
    OrderBook book;
    CommandDispatcher dispatcher(book);

    ParsedCommand cmd = makeOrder(/*id*/ 1);

    auto out = dispatcher.dispatch(cmd);
    EXPECT_EQ(out, "1 - Accept");
}

TEST(DispatcherTests, DispatchNew_DuplicateId_PrintsReject303) {
    OrderBook book;
    CommandDispatcher dispatcher(book);

    ParsedCommand first = makeOrder(7);
    ParsedCommand dup   = makeOrder(7); // same id

    EXPECT_EQ(dispatcher.dispatch(first), "7 - Accept");

    auto out = dispatcher.dispatch(dup);
    // your exact text should match NewCommandHandler::format
    EXPECT_EQ(out, "7 - Reject - 303 - Invalid order details");
}

TEST(DispatcherTests, DispatchCancel_OnMissingOrder_Prints404) {
    OrderBook book;
    CommandDispatcher dispatcher(book);

    CancelRequest req{};
    req.orderId = 999;
    req.timeStamp = 1;

    ParsedCommand cmd = req;

    auto out = dispatcher.dispatch(cmd);
    EXPECT_EQ(out, "999 - CancelReject - 404 - Order does not exist");
}

TEST(DispatcherTests, DispatchCancel_Twice_SecondRejects404) {
    OrderBook book;
    CommandDispatcher dispatcher(book);

    // add order first
    EXPECT_EQ(dispatcher.dispatch(ParsedCommand{makeOrder(2)}), "2 - Accept");

    CancelRequest c1{};
    c1.orderId = 2;
    c1.timeStamp = 10;

    CancelRequest c2 = c1;

    EXPECT_EQ(dispatcher.dispatch(ParsedCommand{c1}), "2 - CancelAccept");

    auto out2 = dispatcher.dispatch(ParsedCommand{c2});
    EXPECT_EQ(out2, "2 - CancelReject - 404 - Order does not exist");
}

TEST(DispatcherTests, DispatchAmend_OnMissingOrder_Prints404) {
    OrderBook book;
    CommandDispatcher dispatcher(book);

    AmendRequest req{};
    req.orderId = 50;
    req.timeStamp = 2;
    req.symbol = "XYZ";
    req.orderType = domain::OrderType::Limit;
    req.side = domain::Side::Buy;
    req.newPrice = 10500;
    req.newQuantity = 150;

    ParsedCommand cmd = req;

    auto out = dispatcher.dispatch(cmd);
    EXPECT_EQ(out, "50 - AmendReject - 404 - Order does not exist");
}