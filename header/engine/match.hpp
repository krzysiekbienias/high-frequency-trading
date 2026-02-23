#pragma once

#include "book/order_book.hpp"

#include <optional>
#include <string>


struct MatchRequest {
    domain::Timestamp timestamp{};
    std::optional<std::string> symbol;  // if empty → match all symbols
};

struct TradeEvent {
    std::string symbol;

    domain::OrderId buyOrderId{};
    domain::OrderId sellOrderId{};

    domain::OrderType buyOrderType{};
    domain::OrderType sellOrderType{};

    int quantity{};
    domain::Price executionPrice{};
};


struct MatchResponse {
    std::vector<TradeEvent> events;
};


class MatchHandler {
public:
    explicit MatchHandler(OrderBook& book);

    MatchResponse execute(const MatchRequest& req);

    // Helper to format output exactly as required by spec
    static std::vector<std::string> format(const MatchResponse& response);

private:
    OrderBook& m_book;


};