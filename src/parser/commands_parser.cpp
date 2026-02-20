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



std::optional<ParsedCommand> parseCommandLine(std::string_view line) {
    std::optional<ParsedCommand> parsedObj;
    std::vector<std::string> tokens=tokenize(line);
    if (tokens.empty() || tokens[0].empty()) return std::nullopt;

    char commandSymbol=tokens[0][0];
    if (commandSymbol!= 'N' && commandSymbol!='A' && commandSymbol!='X' ) {
        return std::nullopt;
    }
    if (commandSymbol=='N') {
        if (tokens.size()!=8) return std::nullopt;
        parsedObj=parseNew(tokens);
        return parsedObj;

    }
    if (commandSymbol=='A') {
        if (tokens.size()!=8) return std::nullopt;
        parsedObj=parseAmendRequest(tokens);
        return parsedObj;

    }

    if (commandSymbol=='X') {
        if (tokens.size()!=3) return std::nullopt;
        parsedObj=parseCancelRequest(tokens);
        return parsedObj;

    }
    return std::nullopt;

}
