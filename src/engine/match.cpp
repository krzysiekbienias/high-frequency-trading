#include "engine/match.hpp"

#include <sstream>

MatchHandler::MatchHandler(OrderBook &book)
    : m_book(book) {
}

MatchResponse MatchHandler::execute(const MatchRequest &req) {
    MatchResponse response;

    // --- match only one symbol ---
    if (req.symbol.has_value()) {
        const std::string& sym = *req.symbol;

        while (true) {
            auto* buyPtr  = m_book.bestBidOrder(sym);
            auto* sellPtr = m_book.bestAskOrder(sym);

            // no liquidity for this symbol on one side
            if (!buyPtr || !sellPtr) break;

            // no cross
            if (buyPtr->price < sellPtr->price) break;

            const int matchedQuantity = std::min(buyPtr->quantity, sellPtr->quantity);
            const domain::Price executionPrice = sellPtr->price; // you assumed sell/ask price

            response.events.push_back(TradeEvent{
                sym,
                buyPtr->orderId,
                sellPtr->orderId,
                buyPtr->orderType,
                sellPtr->orderType,
                matchedQuantity,
                executionPrice
            });

            m_book.consumeBestAsk(matchedQuantity, sym);
            m_book.consumeBestBid(matchedQuantity, sym);
        }

        return response;
    }

    // --- match all symbols (global best bid/ask) ---
    while (m_book.hasBuy() && m_book.hasSell()) {
        auto* buyPtr  = m_book.bestBidOrder();
        auto* sellPtr = m_book.bestAskOrder();
        if (!buyPtr || !sellPtr) break; // defensive

        // no cross
        if (buyPtr->price < sellPtr->price) break;

        const int matchedQuantity = std::min(buyPtr->quantity, sellPtr->quantity);
        const domain::Price executionPrice = sellPtr->price;

        response.events.push_back(TradeEvent{
            buyPtr->symbol,
            buyPtr->orderId,
            sellPtr->orderId,
            buyPtr->orderType,
            sellPtr->orderType,
            matchedQuantity,
            executionPrice
        });

        m_book.consumeBestAsk(matchedQuantity);
        m_book.consumeBestBid(matchedQuantity);
    }

    return response;
}

std::vector<std::string> MatchHandler::format(const MatchResponse &response) {




    std::vector<std::string> out;
    std::string buyPart;
    std::string sellPart;


    for (const auto &event: response.events) {
        std::ostringstream ossBuy;
        std::ostringstream ossSell;
        std::string temp;
        ossBuy<<event.buyOrderId<<","<<domain::toChar(event.buyOrderType)<<","<<event.quantity<<","<<event.executionPrice;
        ossSell<<event.executionPrice<<","<<event.quantity<<","<<domain::toChar(event.sellOrderType)<<","<<event.sellOrderId;
        temp=event.symbol+"|"+ossBuy.str()+"|"+ossSell.str();
        out.push_back(temp);
    }

    return out;
}
