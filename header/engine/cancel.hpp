#pragma once

#include "book/order_book.hpp"
#include "domain/order.hpp"

#include <string>

struct CancelRequest {
    domain::OrderId orderId{};
    domain::Timestamp timeStamp{};
};

struct CancelResponse {
    domain::OrderId orderId{};
    bool accepted{false};

    // 101 - invalid cancel details
    // 404 - order does not exist
    int rejectCode{101};
    std::string rejectMessage{"Invalid cancel details"};
};

class CancelHandler {
public:
    explicit CancelHandler(OrderBook& book);

    CancelResponse execute(const CancelRequest& req);

    static std::string format(const CancelResponse& res);

private:
    bool isValidCancelRequest(const CancelRequest& req) const;
    OrderBook& m_book;
};