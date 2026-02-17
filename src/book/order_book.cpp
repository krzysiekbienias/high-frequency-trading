#include "book/order_book.hpp"

bool OrderBook::contains(domain::OrderId id) const {
    return m_liveIds.find(id) != m_liveIds.end();
}

bool OrderBook::add(const domain::Order& order) {
    if (contains(order.orderId)) {
        return false;
    }

    if (order.side == domain::Side::Buy) {
        m_buyBook[order.price].push_back(order);
    } else {
        m_sellBook[order.price].push_back(order);
    }

    m_liveIds.insert(order.orderId);
    return true;
}

std::size_t OrderBook::liveCount() const {
    return m_liveIds.size();
}

std::size_t OrderBook::buyCount() const {
    std::size_t total = 0;
    for (const auto& [price, q] : m_buyBook) {
        total += q.size();
    }
    return total;
}

std::size_t OrderBook::sellCount() const {
    std::size_t total = 0;
    for (const auto& [price, q] : m_sellBook) {
        total += q.size();
    }
    return total;
}