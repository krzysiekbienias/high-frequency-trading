#pragma once

#include "book/order_book.hpp"
#include "domain/order.hpp"

#include <string>

struct NewCommandResponse {
    //only for store result of the command
    domain::OrderId orderId{};
    bool accepted{false};

    // for reject
    int rejectCode{303};
    std::string rejectMessage{"Invalid order details"};
};

// This class handles ONLY the business logic for N (New) command.
class NewCommandHandler {
public:
    explicit NewCommandHandler(OrderBook& book);

    NewCommandResponse execute(const domain::Order& order) const;

    // helper to format exactly as required
    static std::string format(const NewCommandResponse& r);

private:
    bool isValidNew(const domain::Order& order) const;
    static bool isAlphaSymbol(const std::string& s);

    OrderBook& m_book;
};