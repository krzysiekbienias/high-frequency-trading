#pragma once

#include <optional>
#include <string_view>
#include <variant>

#include "engine/new.hpp"
#include "engine/amend.hpp"
#include "engine/cancel.hpp"

// ParsedCommand = one of supported requests
using ParsedCommand = std::variant<
    domain::Order, //for new we don't have request becasue new introduct new object (order) and requires all fields
    AmendRequest,
    CancelRequest
>;

// Main entry: line -> tokenize -> parse fields -> build request
std::optional<ParsedCommand> parseCommandLine(std::string_view line);