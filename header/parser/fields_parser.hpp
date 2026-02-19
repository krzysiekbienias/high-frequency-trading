#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "domain/types.hpp"   // OrderId, Timestamp, Price, Side, OrderType

// Strict integer parsing (no trailing junk, no decimals).
// Accepts optional leading/trailing spaces (because tokenizer trims, but ok).
std::optional<std::int64_t> parseInt64Strict(std::string_view s);

// Domain-level numeric parsers
std::optional<domain::OrderId>   parseOrderId(std::string_view s);     // > 0
std::optional<domain::Timestamp> parseTimestamp(std::string_view s);   // >= 0
std::optional<int>              parseQuantity(std::string_view s);    // > 0

// Enums
std::optional<domain::Side>      parseSide(std::string_view s);       // "B" or "S"
std::optional<domain::OrderType> parseOrderType(std::string_view s);  // "M","L","I"

// Price parsing:
// - Accept exactly "0.00" or "104.53" style: digits '.' 2 digits
// - Return cents as domain::Price (int64_t) i.e. 10453 for "104.53"
std::optional<domain::Price> parsePriceCents(std::string_view s);