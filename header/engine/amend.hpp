#pragma once

#include "domain/order.hpp"
#include "book/order_book.hpp"

#include <optional>
#include <string>

struct AmendRequest {
    domain::OrderId orderId{};
    domain::Timestamp timeStamp{};

    // Te pola przychodzą w komendzie A i służą do weryfikacji,
    // że nie próbujesz zmieniać "niemodyfikowalnych" pól.
    std::string symbol{};
    domain::OrderType orderType{};
    domain::Side side{};

    // Partial amend:
    // - nullopt => brak zmiany
    // - value   => nowa wartość
    std::optional<domain::Price> newPrice{};
    std::optional<int> newQuantity{};
};

struct AmendResult {
    domain::OrderId orderId{};
    bool accepted{false};

    // 101 - invalid amendment details
    // 404 - order does not exist
    int rejectCode{101};
    std::string rejectMessage{"Invalid amendement details"}; // trzymam pisownię jak w specu
};

class AmendHandler {
public:
    explicit AmendHandler(OrderBook& book);

    AmendResult execute(const AmendRequest& req);

    static std::string format(const AmendResult& r);

private:
    bool isAlphaSymbol(const std::string& s) const;

    // walidacja reguł A (bez parsowania!)
    bool isValidAmendRequest(const AmendRequest& req) const;

private:
    OrderBook& m_book;
};