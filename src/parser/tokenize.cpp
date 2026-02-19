#include "parser/tokenize.hpp"

#include <cctype>

static std::string_view trimView(std::string_view s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }

    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }

    return s.substr(start, end - start);
}

std::vector<std::string> tokenize(std::string_view line) {
    // Strip trailing newline(s)
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
        line.remove_suffix(1);
    }

    std::vector<std::string> out;
    std::size_t start = 0;

    for (std::size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == ',') {
            std::string_view tokenView = line.substr(start, i - start);
            tokenView = trimView(tokenView);

            out.emplace_back(tokenView);
            start = i + 1; // skip comma
        }
    }

    return out;
}