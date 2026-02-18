#include <gtest/gtest.h>

#include "book/order_book.hpp"
#include "engine/amend.hpp"
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
    o.orderId   = id;
    o.timeStamp = ts;
    o.symbol    = symbol;
    o.orderType = type;
    o.side      = side;
    o.price     = priceCents;
    o.quantity  = qty;
    return o;
}

AmendRequest makeAmend(domain::OrderId id,
                       domain::Timestamp ts,
                       const std::string& symbol,
                       domain::OrderType type,
                       domain::Side side,
                       std::optional<domain::Price> newPrice,
                       std::optional<int> newQty) {
    AmendRequest r;
    r.orderId      = id;
    r.timeStamp    = ts;
    r.symbol       = symbol;
    r.orderType    = type;
    r.side         = side;
    r.newPrice     = newPrice;
    r.newQuantity  = newQty;
    return r;
}

} // namespace

TEST(AmendTests, Reject404_WhenOrderDoesNotExist) {
    OrderBook book;
    AmendHandler amend(book);

    auto req = makeAmend(42, 10, "XYZ", domain::OrderType::Limit, domain::Side::Buy,
                         /*newPrice*/ std::nullopt,
                         /*newQty*/  90);

    auto res = amend.execute(req);

    EXPECT_FALSE(res.accepted);
    EXPECT_EQ(res.rejectCode, 404);
    EXPECT_EQ(res.rejectMessage, "Order does not exist");
}

TEST(AmendTests, Reject101_WhenNeitherPriceNorQuantityProvided) {
    OrderBook book;
    AmendHandler amend(book);

    // Add an existing order (so we don't accidentally hit 404)
    book.add(makeOrder(1, 1, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 10000, 100));

    auto req = makeAmend(1, 10, "XYZ", domain::OrderType::Limit, domain::Side::Buy,
                         /*newPrice*/ std::nullopt,
                         /*newQty*/  std::nullopt);

    auto res = amend.execute(req);

    EXPECT_FALSE(res.accepted);
    EXPECT_EQ(res.rejectCode, 101);
}

TEST(AmendTests, Reject101_WhenTryingToChangeSide) {
    OrderBook book;
    AmendHandler amend(book);

    book.add(makeOrder(2, 1, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 10000, 100));

    // Same id, but side changed -> forbidden
    auto req = makeAmend(2, 10, "XYZ", domain::OrderType::Limit, domain::Side::Sell,
                         /*newPrice*/ std::nullopt,
                         /*newQty*/  90);

    auto res = amend.execute(req);

    EXPECT_FALSE(res.accepted);
    EXPECT_EQ(res.rejectCode, 101);
    EXPECT_EQ(res.rejectMessage, "Invalid amendement details");
}

TEST(AmendTests, Reject101_WhenTryingToChangeOrderType) {
    OrderBook book;
    AmendHandler amend(book);

    book.add(makeOrder(3, 1, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 10000, 100));

    // Type changed -> forbidden
    auto req = makeAmend(3, 10, "XYZ", domain::OrderType::IOC, domain::Side::Buy,
                         /*newPrice*/ std::nullopt,
                         /*newQty*/  90);

    auto res = amend.execute(req);

    EXPECT_FALSE(res.accepted);
    EXPECT_EQ(res.rejectCode, 101);
}

TEST(AmendTests, Reject101_WhenTryingToChangeSymbol) {
    OrderBook book;
    AmendHandler amend(book);

    book.add(makeOrder(4, 1, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 10000, 100));

    // Symbol changed -> forbidden
    auto req = makeAmend(4, 10, "ABC", domain::OrderType::Limit, domain::Side::Buy,
                         /*newPrice*/ std::nullopt,
                         /*newQty*/  90);

    auto res = amend.execute(req);

    EXPECT_FALSE(res.accepted);
    EXPECT_EQ(res.rejectCode, 101);
}

TEST(AmendTests, Reject101_WhenNewQuantityIsNonPositive) {
    OrderBook book;
    AmendHandler amend(book);

    book.add(makeOrder(5, 1, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 10000, 100));

    auto req = makeAmend(5, 10, "XYZ", domain::OrderType::Limit, domain::Side::Buy,
                         /*newPrice*/ std::nullopt,
                         /*newQty*/  0);

    auto res = amend.execute(req);

    EXPECT_FALSE(res.accepted);
    EXPECT_EQ(res.rejectCode, 101);
}

TEST(AmendTests, Accept_PartialAmendQuantityDown_UpdatesInPlaceAndKeepsPointer) {
    OrderBook book;
    AmendHandler amend(book);

    book.add(makeOrder(10, 1, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 10000, 100));

    auto* beforePtr = book.getById(10);
    ASSERT_NE(beforePtr, nullptr);
    EXPECT_EQ(beforePtr->quantity, 100);
    EXPECT_EQ(beforePtr->price, 10000);

    // qty down only, no price change -> should be in-place
    auto req = makeAmend(10, 20, "XYZ", domain::OrderType::Limit, domain::Side::Buy,
                         /*newPrice*/ std::nullopt,
                         /*newQty*/  60);

    auto res = amend.execute(req);

    EXPECT_TRUE(res.accepted);

    auto* afterPtr = book.getById(10);
    ASSERT_NE(afterPtr, nullptr);

    // Key point: we didn't erase/reinsert -> address stays the same.
    EXPECT_EQ(afterPtr, beforePtr);

    EXPECT_EQ(afterPtr->quantity, 60);
    EXPECT_EQ(afterPtr->price, 10000);
}

TEST(AmendTests, Accept_PartialAmendPriceOnly_UpdatesOrderPrice) {
    OrderBook book;
    AmendHandler amend(book);

    book.add(makeOrder(11, 1, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 10000, 100));

    auto req = makeAmend(11, 20, "XYZ", domain::OrderType::Limit, domain::Side::Buy,
                         /*newPrice*/ 10100,
                         /*newQty*/  std::nullopt);

    auto res = amend.execute(req);

    EXPECT_TRUE(res.accepted);

    auto* o = book.getById(11);
    ASSERT_NE(o, nullptr);
    EXPECT_EQ(o->price, 10100);
    EXPECT_EQ(o->quantity, 100);
}

TEST(AmendTests, Accept_QuantityUp_MayReinsertAndStillValidState) {
    OrderBook book;
    AmendHandler amend(book);

    book.add(makeOrder(12, 1, "XYZ", domain::OrderType::Limit, domain::Side::Buy, 10000, 100));

    auto* beforePtr = book.getById(12);
    ASSERT_NE(beforePtr, nullptr);

    // qty up -> "other amend" -> your handler likely does erase + add
    auto req = makeAmend(12, 20, "XYZ", domain::OrderType::Limit, domain::Side::Buy,
                         /*newPrice*/ std::nullopt,
                         /*newQty*/  150);

    auto res = amend.execute(req);
    EXPECT_TRUE(res.accepted);

    auto* afterPtr = book.getById(12);
    ASSERT_NE(afterPtr, nullptr);
    EXPECT_EQ(afterPtr->quantity, 150);

    // NOTE: pointer might change because of erase+add; we don't assert inequality (could coincidentally reuse memory).
    // The important thing is the state is correct.
}

TEST(AmendTests, FormatMatchesSpec) {
    AmendResult ok;
    ok.orderId = 7;
    ok.accepted = true;
    EXPECT_EQ(AmendHandler::format(ok), "7 - AmendAccept");

    AmendResult bad;
    bad.orderId = 7;
    bad.accepted = false;
    bad.rejectCode = 404;
    bad.rejectMessage = "Order does not exist";
    EXPECT_EQ(AmendHandler::format(bad), "7 - AmendReject - 404 - Order does not exist");
}