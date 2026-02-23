#include <fstream>
#include <iostream>
#include <string>

#include "book/order_book.hpp"
#include "engine/dispatcher.hpp"
#include "engine/match.hpp"
#include "parser/commands_parser.hpp"  // albo parser/commands_parser.hpp
#include <sstream>
int main(int argc, char** argv) {
    OrderBook book;
    CommandDispatcher dispatcher(book);
    MatchHandler matcher(book);

    std::ifstream file;
    std::istream* in = &std::cin;

    if (argc >= 2) {
        file.open(argv[1]);
        if (!file) {
            std::cerr << "Cannot open file: " << argv[1] << "\n";
            return 1;
        }
        in = &file;
    }

    std::string line;
    while (std::getline(*in, line)) {
        if (line.empty()) continue;

        // DEV shortcut: manual match
        // usage:
        //   :m            -> match all
        //   :m XYZ        -> match only XYZ
        if (line.rfind(":m", 0) == 0) {
            std::istringstream iss(line);
            std::string cmd;
            std::string sym;
            iss >> cmd;      // ":m"
            iss >> sym;      // optional symbol

            MatchRequest req;
            req.timestamp = 0; // dev-only; later: parse real timestamp
            if (!sym.empty()) req.symbol = sym;
            else req.symbol = std::nullopt;

            auto resp = matcher.execute(req);
            auto lines = MatchHandler::format(resp);

            for (const auto& s : lines) {
                std::cout << s << "\n";
            }

            book.dump(std::cout);
            continue;
        }



        if (line == "exit" || line == "quit") break;

        auto parsed = parseCommandLine(line);
        if (!parsed) {
            std::cerr << "[parse] ignored: " << line << "\n";
            continue;
        }

        const std::string out = dispatcher.dispatch(*parsed);
        if (!out.empty()) {
            std::cout << out << "\n";
        }

        book.dump(std::cout);
    }

    return 0;
}