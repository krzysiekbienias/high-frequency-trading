#include <fstream>
#include <iostream>
#include <string>

#include "book/order_book.hpp"
#include "engine/dispatcher.hpp"
#include "parser/commands_parser.hpp"  // albo parser/commands_parser.hpp

int main(int argc, char** argv) {
    OrderBook book;
    CommandDispatcher dispatcher(book);

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