#include "book/order_book.hpp"

bool OrderBook::isLive(domain::OrderId id) const {
    return m_liveIds.find(id) != m_liveIds.end();
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


void OrderBook::dump(std::ostream& os) const {
    os << "=== ORDER BOOK DUMP ===\n";

    os << "BUY (best -> worst)\n";
    if (m_buyBook.empty()) {
        os << "  <empty>\n";
    } else {
        for (const auto& [price, q] : m_buyBook) {
            os << "  price=";
            domain::printPrice(os, price);
            os << " | count=" << q.size() << "\n";
            for (const auto& o : q) {
                os << "    " << o << "\n";
            }
        }
    }

    os << "SELL (best -> worst)\n";
    if (m_sellBook.empty()) {
        os << "  <empty>\n";
    } else {
        for (const auto& [price, q] : m_sellBook) {
            os << "  price=";
            domain::printPrice(os, price);
            os << " | count=" << q.size() << "\n";
            for (const auto& o : q) {
                os << "    " << o << "\n";
            }
        }
    }

    os << "========================\n";
}