#include  "parser/fields_parser.hpp"
#include <system_error>
#include <charconv>
#include <optional>


// Strict integer parsing (no trailing junk, no decimals).

std::optional<std::int64_t> parseInt64Strict(std::string_view s) {
    std::int64_t value=0;
    auto begin=s.data();
    auto end =s.data()+s.size();
    auto result=std::from_chars(begin,end,value);
    bool strictInt=(result.ec==std::errc{}) && (result.ptr==end);
    if (!strictInt) return std::nullopt;
    return value;

}

// Domain-level numeric parsers
std::optional<domain::OrderId> parseOrderId(std::string_view s) {
    //we leverage from parseInt64Strict
    std::optional<std::int64_t> res=parseInt64Strict(s);
    if (res and *res>0 && *res<=INT_MAX) return static_cast<int>(*res);
    return std::nullopt;
}

std::optional<domain::Timestamp> parseTimestamp(std::string_view s) {
    //we leverage from parseInt64Strict
    std::optional<std::int64_t> res=parseInt64Strict(s);
    if (res and *res>=0 && *res<=INT_MAX) return static_cast<int>(*res);
    return std::nullopt;

}

std::optional<int> parseQuantity(std::string_view s) {

    //we leverage from parseInt64Strict
    std::optional<std::int64_t> res=parseInt64Strict(s);
    if (res and *res>0 && *res<=INT_MAX) return static_cast<int>(*res);
    return std::nullopt;
}

// Enums
std::optional<domain::Side> parseSide(std::string_view s) {
    if (s.size() != 1) return std::nullopt;

    char c = s[0];
    if (c == 'B') return domain::Side::Buy;
    if (c == 'S') return domain::Side::Sell;

    return std::nullopt;
}

std::optional<domain::OrderType> parseOrderType(std::string_view s) {
    if (s.size()!=1) {
        return std::nullopt;
    }
    char c=s[0];
    if (c=='M') return domain::OrderType::Market;
    if (c=='L') return domain::OrderType::Limit;
    if (c=='I') return domain::OrderType::IOC;
    return std::nullopt;
}

// Price parsing: "104.53" -> 10453
std::optional<domain::Price> parsePriceCents(std::string_view s) {
    if (s.size()<4) { return std::nullopt;}
    auto pos = s.find('.');
    if (pos==std::string_view::npos) {return std::nullopt;}
    if (pos==0) return std::nullopt; //".53 not valid"
    if (pos+3!=s.size()) return std::nullopt; // "104.5367" only two places after '.'
    auto beforeDot= s.substr(0,pos);
    auto afterDot=s.substr(pos+1,std::string_view::npos);
    auto resBeforeDot=parseInt64Strict(beforeDot);
    auto resAfterDot=parseInt64Strict(afterDot);
    if (resBeforeDot && resAfterDot) {
        int64_t whole=*resBeforeDot;
        int64_t frac=(s[pos+1]-'0')*10+(s[pos+2]-'0');
        int64_t price=whole*100+frac;
        if (price >=0) {
            return price;
        }

    }

    return std::nullopt;
}