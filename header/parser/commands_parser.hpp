#pragma once

#include <optional>
#include <string_view>
#include <variant>

#include "engine/new.hpp"
#include "engine/amend.hpp"
#include "engine/cancel.hpp"
#include "engine/match.hpp"

// ParsedCommand = one of the supported requests
using ParsedCommand = std::variant<
    domain::Order, //for the new command we don't have a request because new introduce new object (order) and requires all fields
    AmendRequest,
    CancelRequest,
    MatchRequest>;

// Main entry: line -> tokenize -> parse fields -> build request
std::optional<ParsedCommand> parseCommandLine(std::string_view line);