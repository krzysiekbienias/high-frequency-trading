#pragma once

#include "domain/order.hpp"

#include <cstddef>        // std::size_t
#include <deque>
#include <functional>     // std::greater
#include <map>
#include <unordered_set>

class OrderBook {
public:
    // Check if an orderId is already live (duplicate prevention)
    bool isLive(domain::OrderId id) const;

    // Try to add a new order. Returns false if duplicate orderId.
    bool add(const domain::Order& order);

    // Helpers for tests / diagnostics
    std::size_t liveCount() const;
    std::size_t buyCount() const;
    std::size_t sellCount() const;

    domain::Order* getById(domain::OrderId id);
    bool erase(domain::OrderId id);

    void dump(std::ostream& os) const;



private:
    using OrderQueue = std::deque<domain::Order>;

    std::unordered_set<domain::OrderId> m_liveIds;

    // price-time priority: each price level keeps FIFO queue
    std::map<domain::Price, OrderQueue, std::greater<domain::Price>> m_buyBook;
    std::map<domain::Price, OrderQueue> m_sellBook;
};