#pragma once

#include <string>
#include <string_view>
#include <vector>

// Splits a single input line into tokens separated by ','.
// - Trims leading/trailing whitespace in each token.
// - Preserves empty tokens (e.g. "A,,1" -> {"A","","1"}).
// - Handles lines ending with '\n' or '\r\n'.
std::vector<std::string> tokenize(std::string_view line);