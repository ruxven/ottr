#pragma once
#include <string>
#include <vector>
#include <istream>
#include <optional>
#include <unordered_set>

namespace ottr {

struct Day;

class ParserLogger {
public:
    ParserLogger() = default;

    bool log_error(const std::string& path, const int line_no, const std::string& msg);
    bool log_warning(const std::string& path, const int line_no, const std::string& msg);
    bool log_debug(const std::string& path, const int line_no, const std::string& msg);

    const std::vector<std::string>& get_errors();
    const std::vector<std::string>& get_warnings();
    const std::vector<std::string>& get_debug();
    bool empty_err() const;
    bool empty_warnings() const;
    bool find_err(const std::string&) const;
private:
    std::vector<std::string> error_message_vector;
    std::vector<std::string> warning_message_vector;
    std::vector<std::string> debug_message_vector;
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
bool parse_file(const std::string& path, struct World& world, ParserLogger& log);

// Stream-based parsing interface; source_name is used in error messages.
// On error, returns false and sets error_message.
bool parse_istream(std::istream& in, const std::string& source_name, struct World& world, ParserLogger& log);


class DirectiveParser {
protected:
    DirectiveParser(
        const std::string& source_name,
        struct World& world,
        ParserLogger& log
    );
    virtual ~DirectiveParser() = default;

    bool ensure_day_closed(const int lno);

    const std::string& source_name;
    struct World& world;
    ParserLogger& log;
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
        struct World& world,
        ParserLogger& log
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
        struct World& world,
        ParserLogger& log
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
        struct World& world,
        ParserLogger& log
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
        struct World& world,
        ParserLogger& log
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
        struct World& world,
        ParserLogger& log
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
        struct World& world,
        ParserLogger& log
    );

    bool parse(
        const int line_no,
        const std::vector<Token>& tokens
    ) override final;

private:
    void warn_override(const int line_no, const std::string& key);
    bool set_rounding(const int line_no, const std::string& value);
    bool set_aggregation(const int line_no, const std::string& value);
    bool set_remainder(const int line_no, const std::string& value);
    bool set_budget(const int line_no, const std::string& value);
    bool set_overflow_charge(const int line_no, const Token& token);
    bool set_granularity(const int line_no, const std::string& value);
    bool set_audit(const int line_no, const std::string& value);

    // Track which keys have been set so we can warn on any duplicate, regardless
    // of whether the repeated value matches the default.
    std::unordered_set<std::string> seen_keys_;
};

} // namespace ottr
