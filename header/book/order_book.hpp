#pragma once

#include "domain/order.hpp"

#include <cstddef>        // std::size_t
#include <deque>
#include <functional>     // std::greater
#include <map>
#include <unordered_set>

class OrderBook {
public:

    bool hasBuy() const;
    bool hasSell() const;

    // Best prices (empty => no orders on that side)
    std::optional<domain::Price> bestBidPrice() const; // highest BUY price
    std::optional<domain::Price> bestAskPrice() const; // lowest  SELL price


    // Access the best (front) order at best price level (FIFO).
    // Returns nullptr if the side is empty.
    domain::Order* bestBidOrder(); // BUY: m_buyBook.begin()->second.front()
    domain::Order* bestAskOrder(); // SELL: m_sellBook.begin()->second.front()

    // Consume quantity from the best/front order.
    // - Decrements qty by matchedQty
    // - If qty reaches 0: pops it from deque and removes id from m_liveIds
    // - If price level becomes empty: removes the price level from the map
    // Precondition: side not empty, matchedQty > 0, matchedQty <= front.qty
    void consumeBestBid(int matchedQty);
    void consumeBestAsk(int matchedQty);


    //the same set of methods overloading to handle with 'symbol' parameter

    std::optional<domain::Price> bestBidPrice(const std::string& symbol) const;
    std::optional<domain::Price> bestAskPrice(const std::string& symbol) const;


    domain::Order* bestBidOrder(const std::string& symbol);
    domain::Order* bestAskOrder(const std::string& symbol);

    void consumeBestBid(int matchedQty,const std::string& symbol);
    void consumeBestAsk(int matchedQty,const std::string& symbol);


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