#pragma once

#include "domain/types.hpp"

#include <string>
#include <ostream>
#include <iomanip>
#include <cstdlib>   // std::llabs

namespace domain {

    struct Order {
        OrderId orderId{};
        Timestamp timeStamp{};
        std::string symbol{};
        OrderType orderType{OrderType::Limit};
        Side side{Side::Buy};
        Price price{};          // cents
        int quantity{};
    };

    // Mały helper tylko do debug-print (jedno źródło prawdy w tym pliku)
    inline void printPrice(std::ostream& os, Price p) {
        const auto whole = p / 100;
        const auto frac  = std::llabs(p % 100);
        os << whole << '.' << std::setw(2) << std::setfill('0') << frac;
        os << std::setfill(' ');
    }

    inline const char* toChar(OrderType t) {
        switch (t) {
            case OrderType::Market: return "M";
            case OrderType::Limit:  return "L";
            case OrderType::IOC:    return "I";
        }
        return "?";
    }

    inline const char* toChar(Side s) {
        return (s == Side::Buy) ? "B" : "S";
    }

    inline std::ostream& operator<<(std::ostream& os, const Order& o) {
        os << "Order{"
           << "id=" << o.orderId
           << ", ts=" << o.timeStamp
           << ", sym=" << o.symbol
           << ", type=" << toChar(o.orderType)
           << ", side=" << toChar(o.side)
           << ", price=";
        printPrice(os, o.price);
        os << ", qty=" << o.quantity
           << "}";
        return os;
    }

} // namespace domain