#pragma once
#include <string>
#include <vector>
#include <istream>

namespace ottr {

struct Token {
    std::string text;
    bool quoted = false;
};

// Tokenize a single line (already stripped of trailing newline) while respecting
// comments beginning with '#' and supporting double-quoted strings with escapes (\").
std::vector<Token> tokenize_line(const std::string& line);

// Parsing interface producing a World; stops at first fatal error.
// On error, returns false and sets error_message.
bool parse_file(const std::string& path, class World& out, std::string& error_message);

// Stream-based parsing interface; source_name is used in error messages.
// On error, returns false and sets error_message.
bool parse_istream(std::istream& in, const std::string& source_name, class World& out, std::string& error_message);

} // namespace ottr
