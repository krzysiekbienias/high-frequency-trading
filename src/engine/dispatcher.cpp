#include "engine/dispatcher.hpp"


CommandDispatcher::CommandDispatcher(OrderBook &book)
    : m_book(book),
      m_new(book),
      m_amend(book),
      m_cancel(book),
      m_match(book)
{
}

std::string CommandDispatcher::dispatch(const ParsedCommand &cmd) {
    if (std::holds_alternative<domain::Order>(cmd)) {
        const auto &payload = std::get<domain::Order>(cmd);
        auto resp = m_new.execute(payload);
        return NewCommandHandler::format(resp);
    }

    if (std::holds_alternative<AmendRequest>(cmd)) {
        const auto &payload = std::get<AmendRequest>(cmd);
        auto resp = m_amend.execute(payload);
        return AmendHandler::format(resp);
    }


    if (std::holds_alternative<CancelRequest>(cmd)) {
        const auto &payload = std::get<CancelRequest>(cmd);
        auto resp = m_cancel.execute(payload);
        return CancelHandler::format(resp);
    }
    return "";

}

//we need a slightly different dispatch for Match becasue it returns vector<string> not string
std::vector<std::string> CommandDispatcher:: dispatchMatch(const ParsedCommand& cmd) {
    const auto &payload = std::get<MatchRequest>(cmd);
    auto resp = m_match.execute(payload);
    return MatchHandler::format(resp);
}






