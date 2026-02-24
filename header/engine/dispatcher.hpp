#pragma once

#include <string>

#include "book/order_book.hpp"

#include "engine/new.hpp"     // NewCommandHandler / NewCommandResponse
#include "engine/amend.hpp"   // AmendCommandHandler / AmendCommandResponse
#include "engine/cancel.hpp"  // CancelCommandHandler / CancelCommandResponse

#include "parser/commands_parser.hpp"  // ParsedCommand

class CommandDispatcher {
public:
    explicit CommandDispatcher(OrderBook& book);

    // Takes a parsed command and returns a formatted output line
    std::string dispatch(const ParsedCommand& cmd);
    std::vector<std::string>dispatchMatch(const ParsedCommand& cmd);

private:
    OrderBook& m_book;

    NewCommandHandler   m_new;
    AmendHandler m_amend;
    CancelHandler m_cancel;
    MatchHandler m_match;
};