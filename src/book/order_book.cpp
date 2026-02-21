#include "book/order_book.hpp"

bool OrderBook::isLive(domain::OrderId id) const {
    return m_liveIds.find(id) != m_liveIds.end();
}


bool OrderBook::hasBuy() const {
    return !m_buyBook.empty();
}

bool OrderBook::hasSell() const {
    return !m_sellBook.empty();
}

std::optional<domain::Price> OrderBook::bestBidPrice() const {
    if (hasBuy()) {
        return m_buyBook.begin()->first;
    }
    return std::nullopt;
}


std::optional<domain::Price> OrderBook::bestBidPrice(const std::string &symbol) const {
    if (hasBuy()) {
        for (const auto &items: m_buyBook) {
            const auto &q = items.second;
            for (const auto &o: q) {
                if (o.symbol == symbol) {
                    return items.first;
                }
            }
        }
        return std::nullopt;
    }
    return std::nullopt;
}


std::optional<domain::Price> OrderBook::bestAskPrice() const {
    if (hasSell()) {
        return m_sellBook.begin()->first;
    }
    return std::nullopt;
}


std::optional<domain::Price> OrderBook::bestAskPrice(const std::string &symbol) const {
    if (hasSell()) {
        for (const auto &items: m_sellBook) {
            const auto &q = items.second;
            for (const auto &o: q) {
                if (o.symbol == symbol) {
                    return items.first;
                }
            }
        }
        return std::nullopt;
    }
    return std::nullopt;
}


domain::Order *OrderBook::bestBidOrder() {
    if (hasBuy()) {
        return &m_buyBook.begin()->second.front();
    }
    return nullptr;
}

domain::Order *OrderBook::bestBidOrder(const std::string &symbol) {
    if (!hasBuy()) return nullptr;

    for (auto &items: m_buyBook) {
        auto &q = items.second;
        for (auto &o: q) {
            if (o.symbol == symbol) {
                return &o;
            }
        }
    }
    return nullptr;
}


domain::Order *OrderBook::bestAskOrder() {
    if (hasSell()) {
        return &m_sellBook.begin()->second.front();
    }
    return nullptr;
}

domain::Order *OrderBook::bestAskOrder(const std::string &symbol) {
    if (!hasSell()) return nullptr;

    for (auto &items: m_sellBook) {
        auto &q = items.second;
        for (auto &o: q) {
            if (o.symbol == symbol) {
                return &o;
            }
        }
    }
    return nullptr;
}


void OrderBook::consumeBestBid(int matchedQty) {
    auto *ord = bestBidOrder();
    if (!ord) {
        //to avoid risk of nullptr
        return;
    }
    if (hasBuy() && matchedQty > 0 && matchedQty <= ord->quantity) {
        auto it = m_buyBook.begin(); //--> iterator to the best level
        auto &q = it->second;

        ord->quantity -= matchedQty;
        if (ord->quantity == 0) {
            auto id = ord->orderId; //extract becasue it will disapear for a moment
            q.pop_front();
            m_liveIds.erase(id);
            if (q.empty()) {
                //there are no more levels for the same price
                //we may remove level from the map (order_book)
                m_buyBook.erase(it);
            }
        }
    }
}

void OrderBook::consumeBestAsk(int matchedQty) {
    auto *ord = bestAskOrder();
    if (!ord) {
        //to avoid risk of nullptr
        return;
    }
    if (hasSell() && matchedQty > 0 && matchedQty <= ord->quantity) {
        auto it = m_sellBook.begin(); //--> iterator to the best level
        auto &q = it->second;

        ord->quantity -= matchedQty;
        if (ord->quantity == 0) {
            auto id = ord->orderId; //extract becasue it will disapear for a moment
            q.pop_front();
            m_liveIds.erase(id);
            if (q.empty()) {
                //there are no more levels for the same price
                //we may remove level from the map (order_book)
                m_sellBook.erase(it);
            }
        }
    } else return;
}


void OrderBook::consumeBestBid(int matchedQty, const std::string &symbol) {
    if (matchedQty <= 0) return;
    if (!hasBuy()) return;
    // Iterate BUY price levels from best to worst (map is already sorted best->worst)
    for (auto levelIt = m_buyBook.begin(); levelIt != m_buyBook.end(); ++levelIt) {
        auto &q = levelIt->second;
        //find first FIFO order in this price level they matches the symbol
        for (auto ordIt = q.begin(); ordIt != q.end(); ++ordIt) {
            if (ordIt->symbol != symbol) {
                continue;
            }
            //Found the order to consume
            if (matchedQty > ordIt->quantity) {
                // This should not happen if matcher computed matchedQty correctly.
                // MVP: just ignore.
                return;
            }
            ordIt->quantity -= matchedQty;
            if (ordIt->quantity == 0) {
                const auto id = ordIt->orderId;
                // Remove the order from this price level queue
                q.erase(ordIt);
                m_liveIds.erase(id);
                // If the whole price level is empty, remove the level from the map
                if (q.empty()) {
                    m_buyBook.erase(levelIt);
                }
            }
            return; //consumed exactly one order
        }
        // If we reach here: no BUY order for given symbol was found -> nothing to consume
    }
}


void OrderBook::consumeBestAsk(int matchedQty, const std::string &symbol) {
    if (matchedQty <= 0) return;
    if (!hasSell()) return;
    // Iterate BUY price levels from best to worst (map is already sorted best->worst)
    for (auto levelIt = m_sellBook.begin(); levelIt != m_sellBook.end(); ++levelIt) {
        auto &q = levelIt->second;
        //find first FIFO order in this price level they matches the symbol
        for (auto ordIt = q.begin(); ordIt != q.end(); ++ordIt) {
            if (ordIt->symbol != symbol) {
                continue;
            }
            //Found the order to consume
            if (matchedQty > ordIt->quantity) {
                // This should not happen if matcher computed matchedQty correctly.
                // MVP: just ignore.
                return;
            }
            ordIt->quantity -= matchedQty;
            if (ordIt->quantity == 0) {
                const auto id = ordIt->orderId;
                // Remove the order from this price level queue
                q.erase(ordIt);
                m_liveIds.erase(id);
                // If the whole price level is empty, remove the level from the map
                if (q.empty()) {
                    m_sellBook.erase(levelIt);
                }
            }
            return; //consumed exactly one order
        }
        // If we reach here: no BUY order for given symbol was found -> nothing to consume
    }
}


bool OrderBook::add(const domain::Order &order) {
    if (isLive(order.orderId)) {
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
    for (const auto &[price, q]: m_buyBook) {
        total += q.size();
    }
    return total;
}

std::size_t OrderBook::sellCount() const {
    std::size_t total = 0;
    for (const auto &[price, q]: m_sellBook) {
        total += q.size();
    }
    return total;
}

domain::Order *OrderBook::getById(domain::OrderId id) {
    // scan buy side
    for (auto &[price, q]: m_buyBook) {
        for (auto &ord: q) {
            if (ord.orderId == id) {
                return &ord;
            }
        }
    }

    // scan sell side
    for (auto &[price, q]: m_sellBook) {
        for (auto &ord: q) {
            if (ord.orderId == id) {
                return &ord;
            }
        }
    }

    return nullptr;
}

bool OrderBook::erase(domain::OrderId id) {
    // buy side
    for (auto levelIt = m_buyBook.begin(); levelIt != m_buyBook.end(); ++levelIt) {
        auto &q = levelIt->second;

        for (auto it = q.begin(); it != q.end(); ++it) {
            if (it->orderId == id) {
                q.erase(it);

                // remove empty price level
                if (q.empty()) {
                    m_buyBook.erase(levelIt);
                }

                m_liveIds.erase(id);
                return true;
            }
        }
    }

    // sell side
    for (auto levelIt = m_sellBook.begin(); levelIt != m_sellBook.end(); ++levelIt) {
        auto &q = levelIt->second;

        for (auto it = q.begin(); it != q.end(); ++it) {
            if (it->orderId == id) {
                q.erase(it);

                if (q.empty()) {
                    m_sellBook.erase(levelIt);
                }

                m_liveIds.erase(id);
                return true;
            }
        }
    }

    return false;
}


void OrderBook::dump(std::ostream &os) const {
    os << "=== ORDER BOOK DUMP ===\n";

    os << "BUY (highest -> lowest)\n";
    if (m_buyBook.empty()) {
        os << "  <empty>\n";
    } else {
        for (const auto &[price, q]: m_buyBook) {
            os << "  price=";
            domain::printPrice(os, price);
            os << " | count=" << q.size() << "\n";
            for (const auto &o: q) {
                os << "    " << o << "\n";
            }
        }
    }

    os << "SELL (lowest -> highest)\n";
    if (m_sellBook.empty()) {
        os << "  <empty>\n";
    } else {
        for (const auto &[price, q]: m_sellBook) {
            os << "  price=";
            domain::printPrice(os, price);
            os << " | count=" << q.size() << "\n";
            for (const auto &o: q) {
                os << "    " << o << "\n";
            }
        }
    }

    os << "========================\n";
}
