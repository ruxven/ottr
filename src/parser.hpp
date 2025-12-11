#pragma once
#include <string>
#include <vector>
#include <istream>
#include <optional>

namespace ottr {

class Day;

class ErrorLogger {
public:
    ErrorLogger() = default;

    bool parse_error(const std::string& path, const int line_no, const std::string& msg);

    const std::vector<std::string>& get_errors();
    bool empty() const;
    bool find(const std::string&) const;
private:
    std::vector<std::string> error_message_vector;
};

struct Token {
    std::string text;
    bool quoted = false;
};

// Tokenize a single line (already stripped of trailing newline) while respecting
// comments beginning with '#' and supporting double-quoted strings with escapes (\").
std::vector<Token> tokenize_line(const std::string& line);

// Parsing interface producing a World; stops at first fatal error.
// On error, returns false and sets error_message.
bool parse_file(const std::string& path, struct World& out, ErrorLogger& err);

// Stream-based parsing interface; source_name is used in error messages.
// On error, returns false and sets error_message.
bool parse_istream(std::istream& in, const std::string& source_name, struct World& out, ErrorLogger& err);


class DirectiveParser {
protected:
    DirectiveParser(
        const std::string& source_name,
        struct World& out,
        ErrorLogger& err,
        std::optional<Day>& current_day
    );
    virtual ~DirectiveParser() = default;

    bool ensure_day_closed(const int lno);

    const std::string& source_name;
    struct World& out;
    ErrorLogger& err;
    std::optional<Day>& current_day;
public:
    virtual bool parse(
        const int line_no,
        const std::vector<Token>& tokens
    ) = 0;
};

class ChargeDirectiveParser : public DirectiveParser {
public:
    ChargeDirectiveParser(
        const std::string& source_name,
        struct World& out,
        ErrorLogger& err,
        std::optional<Day>& current_day
    );

    bool parse(
        const int line_no,
        const std::vector<Token>& tokens
    ) override final;
};

class TaskDirectiveParser : public DirectiveParser {
public:
    TaskDirectiveParser(
        const std::string& source_name,
        struct World& out,
        ErrorLogger& err,
        std::optional<Day>& current_day
    );

    bool parse(
        const int line_no,
        const std::vector<Token>& tokens
    ) override final;
};

class WeightDirectiveParser : public DirectiveParser {
public:
    WeightDirectiveParser(
        const std::string& source_name,
        struct World& out,
        ErrorLogger& err,
        std::optional<Day>& current_day
    );

    bool parse(
        const int line_no,
        const std::vector<Token>& tokens
    ) override final;
};

class DayDirectiveParser : public DirectiveParser {
public:
    DayDirectiveParser(
        const std::string& source_name,
        struct World& out,
        ErrorLogger& err,
        std::optional<Day>& current_day
    );

    bool parse(
        const int line_no,
        const std::vector<Token>& tokens
    ) override final;
};

class LogDirectiveParser : public DirectiveParser {
public:
    LogDirectiveParser(
        const std::string& source_name,
        struct World& out,
        ErrorLogger& err,
        std::optional<Day>& current_day
    );

    bool parse(
        const int line_no,
        const std::vector<Token>& tokens
    ) override final;
};

class OptionDirectiveParser : public DirectiveParser {
public:
    OptionDirectiveParser(
        const std::string& source_name,
        struct World& out,
        ErrorLogger& err,
        std::optional<Day>& current_day
    );

    bool parse(
        const int line_no,
        const std::vector<Token>& tokens
    ) override final;
};

} // namespace ottr
