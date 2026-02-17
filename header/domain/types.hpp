// header/domain/types.hpp
#pragma once

#include <cstdint>   // int64_t

namespace domain {

    // --- Fundamental IDs / timestamps ---
    using OrderId   = int;
    using Timestamp = std::int64_t;

    // --- Money / price ---
    // Price stored in "cents" (2 decimal places). Example: 104.53 => 10453
    using Price = std::int64_t;

    // --- Enums used across the engine ---
    enum class Side : std::uint8_t {
        Buy,
        Sell
    };

    enum class OrderType : std::uint8_t {
        Market,
        Limit,
        IOC
    };

} // namespace domain