#include "parser/commands_parser.hpp"

#include "parser/fields_parser.hpp"
#include "parser/tokenize.hpp"

//remeber that parsing also includes builing object/data_structure
static std::optional<domain::Order> parseNew(const std::vector<std::string> & tokens) {
    auto id=parseOrderId(tokens[1]);
    if (!id) return std::nullopt;

    auto ts=parseTimestamp(tokens[2]);
    if (!ts) return std::nullopt;

    //there is no sperwate method to check symbols add later!
    auto tickerSymbol=tokens[3];

    auto orderType=parseOrderType(tokens[4]);
    if (!orderType) return std::nullopt;

    auto side=parseSide(tokens[5]);
    if (!side) return std::nullopt;

    auto price=parsePriceCents(tokens[6]);
    if (!price) return std::nullopt;

    auto quantity=parseQuantity(tokens[7]);
    if (!quantity) return std::nullopt;

    domain::Order order(*id,*ts,tickerSymbol,*orderType,*side,*price,*quantity);
    return order;

}


static std::optional<AmendRequest> parseAmendRequest(const std::vector<std::string> & tokens) {
    auto id=parseOrderId(tokens[1]);
    if (!id) return std::nullopt;

    auto ts=parseTimestamp(tokens[2]);
    if (!ts) return std::nullopt;

    //there is no sperwate method to check symbols add later!
    std::string tickerSymbol=tokens[3];

    auto orderType=parseOrderType(tokens[4]);
    if (!orderType) return std::nullopt;

    auto side=parseSide(tokens[5]);
    if (!side) return std::nullopt;

    auto price=parsePriceCents(tokens[6]);
    if (!price) return std::nullopt;

    auto quantity=parseQuantity(tokens[7]);
    if (!quantity) return std::nullopt;

    AmendRequest amendReq(*id,*ts,tickerSymbol,*orderType,*side,*price,*quantity);
    return amendReq;

}


static std::optional<CancelRequest> parseCancelRequest(const std::vector<std::string> & tokens) {
    auto id=parseOrderId(tokens[1]);
    if (!id) return std::nullopt;

    auto ts=parseTimestamp(tokens[2]);
    if (!ts) return std::nullopt;
    return CancelRequest(*id,*ts);
}

static std::optional<MatchRequest> parseMatchRequest(const std::vector<std::string> & tokens) {
    if (tokens.size()==2) {
        auto ts=parseTimestamp(tokens[1]);
        if (!ts ) return std::nullopt;
        return MatchRequest(*ts);
    }
    if (tokens.size()==3) {
        auto ts=parseTimestamp(tokens[1]);
        auto symbol=tokens[2];
        if (!ts || symbol.empty()) return std::nullopt;
        return MatchRequest(*ts,symbol);
    }
    return std::nullopt;

}


std::optional<ParsedCommand> parseCommandLine(std::string_view line) {
    std::optional<ParsedCommand> parsedObj;
    std::vector<std::string> tokens=tokenize(line);
    if (tokens.empty() || tokens[0].empty()) return std::nullopt;

    char commandSymbol=tokens[0][0];
    if (commandSymbol!= 'N' && commandSymbol!='A' && commandSymbol!='X' && commandSymbol!='M') {
        return std::nullopt;
    }
    switch (commandSymbol) {
        case 'N': {
            if (tokens.size() != 8) return std::nullopt;
            parsedObj = parseNew(tokens);
            return parsedObj;
        }
        case 'A': {
            if (tokens.size() != 8) return std::nullopt;
            parsedObj = parseAmendRequest(tokens);
            return parsedObj;
        }
        case 'X': {
            if (tokens.size() != 3) return std::nullopt;
            parsedObj = parseCancelRequest(tokens);
            return parsedObj;
        }
        case 'M': {
            // keep your correct size check here, e.g.:
            // if (tokens.size() != 2 && tokens.size() != 3) return std::nullopt;
            parsedObj = parseMatchRequest(tokens);
            return parsedObj;
        }
        default:
            return std::nullopt; // logically unreachable, but keeps compiler/IDE happy
    }

}
