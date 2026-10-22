#include "parser.hpp"
#include "model.hpp"
#include "util.hpp"
#include "options.hpp"
#include <fstream>
#include <sstream>
#include <cctype>
#include <iostream>

namespace ottr {

namespace {

void push_token(std::vector<Token>& tokens, std::string& cur, bool quoted,
                bool keep_empty = false) {
    if (keep_empty || !cur.empty()) tokens.push_back(Token{cur, quoted});
    cur.clear();
}

// Consume one character of a line. Returns true when the rest of the line is a
// comment and tokenizing should stop.
bool consume_char(const char c, bool& in_string, bool& escape, std::string& cur,
                  std::vector<Token>& tokens) {
    if (!in_string) {
        if (c == '#') return true;
        if (c == '"') {
            push_token(tokens, cur, false);
            in_string = true;
            return false;
        }
        if (std::isspace(static_cast<unsigned char>(c))) {
            push_token(tokens, cur, false);
            return false;
        }
        cur.push_back(c);
        return false;
    }

    if (escape) {
        cur.push_back(c);
        escape = false;
        return false;
    }
    if (c == '\\') {
        escape = true;
        return false;
    }
    if (c == '"') {
        push_token(tokens, cur, true, true);
        in_string = false;
        return false;
    }
    cur.push_back(c);
    return false;
}

} // namespace

std::vector<Token> tokenize_line(const std::string& line) {
    std::vector<Token> tokens;
    std::string cur;
    bool in_string = false;
    bool escape = false;

    for (const char c : line) {
        if (consume_char(c, in_string, escape, cur, tokens)) break;
    }
    if (!in_string) push_token(tokens, cur, false);
    // Unterminated string: treat whatever gathered as a quoted token for parser
    // error detection.
    if (in_string) push_token(tokens, cur, true, true);
    return tokens;
}

bool ParserLogger::log_debug(const std::string& path, const int line_no, const std::string& msg) {
    std::ostringstream oss;
    oss << path << ":" << line_no << ": " << msg;
    debug_message_vector.push_back(oss.str());
    return false;
}
bool ParserLogger::log_error(const std::string& path, const int line_no, const std::string& msg) {
    std::ostringstream oss;
    oss << path << ":" << line_no << ": " << msg;
    error_message_vector.push_back(oss.str());
    return false;
}
bool ParserLogger::log_warning(const std::string& path, const int line_no, const std::string& msg) {
    std::ostringstream oss;
    oss << path << ":" << line_no << ": warning: " << msg;
    warning_message_vector.push_back(oss.str());
    return true;
}

const std::vector<std::string>& ParserLogger::get_errors(){
    return error_message_vector;
}

const std::vector<std::string>& ParserLogger::get_warnings(){
    return warning_message_vector;
}

const std::vector<std::string>& ParserLogger::get_debug(){
    return debug_message_vector;
}

bool ParserLogger::empty_err() const{
    return error_message_vector.empty();
}

bool ParserLogger::empty_warnings() const{
    return warning_message_vector.empty();
}

bool ParserLogger::find_err(const std::string& q) const
{
    for (size_t i = 0; i < error_message_vector.size(); i++)
    {
        const std::string& err_str = error_message_vector.at(i);
        auto itr = err_str.find(q);
        if (itr != std::string::npos){
            return true;
        }
    }
    return false;
}

bool parse_istream(std::istream& in, const std::string& source_name, World& world, ParserLogger& log) {
    std::string line;
    int line_no = 0;

    log.log_debug(__FUNCTION__, __LINE__, "don't panic");

    ChargeDirectiveParser cn_parser(source_name,world,log);
    TaskDirectiveParser task_parser(source_name,world,log);
    WeightDirectiveParser wt_parser(source_name,world,log);
    DayDirectiveParser day_parser(source_name,world,log);
    LogDirectiveParser log_parser(source_name,world,log);
    OptionDirectiveParser opt_parser(source_name,world,log);

    std::map<std::string, DirectiveParser&> m;
    m.insert({"cn", cn_parser});
    m.insert({"task", task_parser});
    m.insert({"wt", wt_parser});
    m.insert({"day", day_parser});
    m.insert({"log", log_parser});
    m.insert({"opt", opt_parser});

    bool parse_status = true;

    while (std::getline(in, line)) {
        ++line_no;
        auto tokens = tokenize_line(line);
        if (tokens.empty()) continue;
        const std::string& head = tokens[0].text;
        auto m_itr = m.find(head);
        if (m_itr == m.end())
        {
            return log.log_error(source_name, line_no, "unknown directive: " + head);
        }
        //else
        parse_status = m_itr->second.parse(line_no, tokens);

        if (!parse_status){
            return false;
        }
    }

    // Ensure final day is closed
    if (!world.days.empty() && 
        !world.current_day().logs.empty() && 
        world.current_day().logs.back().kind != LogKind::None)
    {
        return log.log_error(source_name, line_no, "file ended but last day not closed with final uncharged log");
    }

    return true;
}

bool parse_file(const std::string& path, World& world, ParserLogger& log) {
    std::ifstream in(path);
    if (!in) {
        log.log_error("parse_file", __LINE__, "unable to open file: " + path);
        return false;
    }
    return parse_istream(in, path, world, log);
}

DirectiveParser::DirectiveParser(
        const std::string& source_name,
        struct World& world,
        ParserLogger& log):
source_name(source_name),
world(world),
log(log)
{
}

ChargeDirectiveParser::ChargeDirectiveParser(
        const std::string& source_name,
        struct World& world,
	ParserLogger& log)
    : DirectiveParser(source_name, world, log) {}

TaskDirectiveParser::TaskDirectiveParser(
        const std::string& source_name,
        struct World& world,
        ParserLogger& log)
    : DirectiveParser(source_name, world, log) {}

WeightDirectiveParser::WeightDirectiveParser(
        const std::string& source_name,
        struct World& world,
        ParserLogger& log)
    : DirectiveParser(source_name, world, log) {}

DayDirectiveParser::DayDirectiveParser(
        const std::string& source_name,
        struct World& world,
        ParserLogger& log)
    : DirectiveParser(source_name, world, log) {}

LogDirectiveParser::LogDirectiveParser(
        const std::string& source_name,
        struct World& world,
        ParserLogger& log)
    : DirectiveParser(source_name, world, log) {}

OptionDirectiveParser::OptionDirectiveParser(
        const std::string& source_name,
        struct World& world,
        ParserLogger& log)
    : DirectiveParser(source_name, world, log) {}

bool ChargeDirectiveParser::parse(const int line_no, const std::vector<Token>& tokens)
{
    if (tokens.size() < 4) return log.log_error(source_name, line_no, "cn requires: cn <id> \"<description>\" <hours> [<priority>]");
    const std::string id = tokens[1].text;
    if (!tokens[2].quoted) return log.log_error(source_name, line_no, "cn description must be quoted");
    const std::string desc = tokens[2].text;
    double hours;
    if (!try_parse_double(tokens[3].text, hours)) return log.log_error(source_name, line_no, "invalid hours in cn");
    long long budget_ticks = hours_to_ticks(hours);
    int prio = 0;
    if (tokens.size() >= 5) {
        long long tmp;
        if (!try_parse_ll(tokens[4].text, tmp)) return log.log_error(source_name, line_no, "invalid priority in cn");
        prio = static_cast<int>(tmp);
    }
    if (world.charges.find(id) != world.charges.end()) return log.log_error(source_name, line_no, "duplicate charge id: " + id);
    ChargeNumber cn;
    cn.id = id;
    cn.description = desc;
    cn.budget_ticks = budget_ticks;
    cn.priority = prio;
    world.charges.emplace(id, std::move(cn));
    return true;
}

bool TaskDirectiveParser::parse(const int line_no, const std::vector<Token>& tokens)
{
    if (tokens.size() < 3) return log.log_error(source_name, line_no, "task requires: task <name> \"<description>\"");
    const std::string name = tokens[1].text;
    if (!tokens[2].quoted) return log.log_error(source_name, line_no, "task description must be quoted");
    if (world.tasks.find(name) != world.tasks.end()) return log.log_error(source_name, line_no, "duplicate task: " + name);
    Task t;
    t.name = name;
    t.description = tokens[2].text;
    world.tasks.emplace(name, std::move(t));
    return true;
}

bool WeightDirectiveParser::parse(const int line_no, const std::vector<Token>& tokens)
{
    if (tokens.size() < 4) return log.log_error(source_name, line_no, "wt requires: wt <task_name> <charge_id> <weight>");
    const std::string task_name = tokens[1].text;
    const std::string charge_id = tokens[2].text;
    long long wll;
    if (!try_parse_ll(tokens[3].text, wll) || wll <= 0) return log.log_error(source_name, line_no, "invalid weight in wt");
    // Allow forward references; validation will check existence.
    auto& task = world.tasks[task_name];
    if (task.name.empty()) task.name = task_name; // placeholder if undeclared
    task.weights[charge_id] += static_cast<int>(wll);
    return true;
}

bool DayDirectiveParser::parse(const int line_no, const std::vector<Token>& tokens)
{
    if (tokens.size() < 2) return log.log_error(source_name, line_no, "day requires: day <MM/DD>");
    if (!ensure_day_closed(line_no)) return false;
    Date d;
    if (!parse_mmdd(tokens[1].text, d)) return log.log_error(source_name, line_no, "invalid date MM/DD");
    // non-decreasing order
    if (!world.days.empty() && d.ord < world.days.back().date.ord) return log.log_error(source_name, line_no, "days out of order (must be non-decreasing)");
    Day day;
    day.date = d;
    world.days.push_back(std::move(day));
    return true;
}

bool LogDirectiveParser::parse(const int line_no, const std::vector<Token>& tokens)
{
    if (world.days.empty()) return log.log_error(source_name, line_no, "log before any day declared");
    if (tokens.size() < 2) return log.log_error(source_name, line_no, "log requires: log <time> [<task_or_charge>] [\"<description>\"]");
    double th;
    if (!try_parse_double(tokens[1].text, th)) return log.log_error(source_name, line_no, "invalid time in log");
    Tick t = hours_to_ticks(th);
    if (!world.current_day().logs.empty() && !(world.current_day().logs.back().time_ticks < t)) return log.log_error(source_name, line_no, "non-increasing log time");
    Log lg;
    lg.time_ticks = t;
    lg.line_no = line_no;
    if (tokens.size() >= 3 && !tokens[2].quoted) {
        lg.ref_id = tokens[2].text;
        // classify later in validation or attempt now
        if (world.tasks.find(lg.ref_id) != world.tasks.end()) lg.kind = LogKind::Task;
        else if (world.charges.find(lg.ref_id) != world.charges.end()) lg.kind = LogKind::Charge;
        else lg.kind = LogKind::Task; // default assume task; validator will catch if unknown
        if (tokens.size() >= 4) {
            if (!tokens[3].quoted) return log.log_error(source_name, line_no, "log description must be quoted if present");
            lg.description = tokens[3].text;
        }
    } else {
        lg.kind = LogKind::None;
    }
    world.current_day().logs.push_back(std::move(lg));
    return true;
}

void OptionDirectiveParser::warn_override(const int line_no, const std::string& key) {
    log.log_warning(source_name, line_no,
        "option '" + key + "' specified more than once; last value wins");
}

bool OptionDirectiveParser::set_rounding(const int line_no, const std::string& value) {
    RoundingMode mode;
    std::string error_msg;
    if (!CalculationOptions::parse_rounding(value, mode, error_msg)) {
        return log.log_error(source_name, line_no, error_msg);
    }
    world.options.rounding = mode;
    return true;
}

bool OptionDirectiveParser::set_aggregation(const int line_no, const std::string& value) {
    AggregationMode mode;
    std::string error_msg;
    if (!CalculationOptions::parse_aggregation(value, mode, error_msg)) {
        return log.log_error(source_name, line_no, error_msg);
    }
    world.options.aggregation = mode;
    return true;
}

bool OptionDirectiveParser::set_remainder(const int line_no, const std::string& value) {
    RemainderPolicy policy;
    std::string error_msg;
    if (!CalculationOptions::parse_remainder(value, policy, error_msg)) {
        return log.log_error(source_name, line_no, error_msg);
    }
    world.options.remainder = policy;
    return true;
}

bool OptionDirectiveParser::set_budget(const int line_no, const std::string& value) {
    BudgetPolicy policy;
    std::string error_msg;
    if (!CalculationOptions::parse_budget(value, policy, error_msg)) {
        return log.log_error(source_name, line_no, error_msg);
    }
    world.options.budget = policy;
    return true;
}

bool OptionDirectiveParser::set_overflow_charge(const int line_no, const Token& token) {
    if (token.quoted) {
        return log.log_error(source_name, line_no,
            "overflow_charge must be a charge id, not a quoted string");
    }
    world.options.overflow_charge = token.text;
    return true;
}

bool OptionDirectiveParser::set_granularity(const int line_no, const std::string& value) {
    Tick ticks;
    std::string error_msg;
    if (!CalculationOptions::parse_granularity(value, ticks, error_msg)) {
        return log.log_error(source_name, line_no, error_msg);
    }
    world.options.granularity = ticks;
    return true;
}

bool OptionDirectiveParser::set_audit(const int line_no, const std::string& value) {
    bool enabled = false;
    std::string error_msg;
    if (!CalculationOptions::parse_flag(value, enabled, error_msg)) {
        return log.log_error(source_name, line_no, error_msg);
    }
    world.options.audit = enabled;
    return true;
}

bool OptionDirectiveParser::parse(const int line_no, const std::vector<Token>& tokens)
{
    if (tokens.size() < 3) return log.log_error(source_name, line_no, "opt requires: opt <key> <value>");
    const std::string& opt_key = tokens[1].text;
    const std::string& opt_value = tokens[2].text;

    bool applied = false;
    if (opt_key == "rounding") {
        applied = set_rounding(line_no, opt_value);
    } else if (opt_key == "aggregation") {
        applied = set_aggregation(line_no, opt_value);
    } else if (opt_key == "remainder") {
        applied = set_remainder(line_no, opt_value);
    } else if (opt_key == "budget") {
        applied = set_budget(line_no, opt_value);
    } else if (opt_key == "overflow_charge") {
        applied = set_overflow_charge(line_no, tokens[2]);
    } else if (opt_key == "granularity") {
        applied = set_granularity(line_no, opt_value);
    } else if (opt_key == "audit") {
        applied = set_audit(line_no, opt_value);
    } else {
        return log.log_error(source_name, line_no,
            "unknown option key: '" + opt_key +
            "'. Valid keys: rounding, aggregation, remainder, budget, "
            "overflow_charge, granularity, audit");
    }

    if (!applied) return false;
    if (seen_keys_.count(opt_key) > 0) warn_override(line_no, opt_key);
    seen_keys_.insert(opt_key);
    return true;
}

bool DirectiveParser::ensure_day_closed(const int lno) {
    if (world.days.empty()) return true;
    const Day& current_day = world.current_day();
    if (current_day.logs.empty()) return true;
    if (current_day.logs.back().kind != LogKind::None) {
        log.log_error(source_name, lno, "day started before previous day closed (missing final uncharged log)");
	return false;
    }
    return true;
}

} // namespace ottr
