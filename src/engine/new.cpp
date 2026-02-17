#include "engine/new.hpp"

#include <cctype>     // std::isalpha
#include <sstream>

NewCommandHandler::NewCommandHandler(OrderBook& book)
    : m_book(book) {}

bool NewCommandHandler::isAlphaSymbol(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char ch : s) {
        if (!std::isalpha(ch)) return false;
    }
    return true;
}

bool NewCommandHandler::isValidNew(const domain::Order& o) const {
    if (o.orderId <= 0) return false;
    if (o.timeStamp < 0) return false;
    if (o.quantity <= 0) return false;
    if (!isAlphaSymbol(o.symbol)) return false;

    // Market: price must be 0
    if (o.orderType == domain::OrderType::Market) {
        return o.price == 0;
    }

    // Limit / IOC: price must be > 0
    return o.price > 0;
}

NewCommandResult NewCommandHandler::execute(const domain::Order& order) const {
    NewCommandResult r;
    r.orderId = order.orderId;

    // 1) validate fields
    if (!isValidNew(order)) {
        r.accepted = false;
        return r; // reject 303
    }

    // 2) reject duplicates
    if (!m_book.add(order)) {
        r.accepted = false;
        return r; // today: duplicate maps to same 303
    }

    // 3) accept
    r.accepted = true;
    return r;
}

std::string NewCommandHandler::format(const NewCommandResult& r) {
    std::ostringstream oss;
    if (r.accepted) {
        oss << r.orderId << " - Accept";
    } else {
        oss << r.orderId << " - Reject - " << r.rejectCode << " - " << r.rejectMessage;
    }
    return oss.str();
}