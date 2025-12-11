#include "parser.hpp"
#include "model.hpp"
#include "util.hpp"
#include <fstream>
#include <sstream>
#include <cctype>

namespace ottr {

std::vector<Token> tokenize_line(const std::string& line) {
    std::vector<Token> tokens;
    std::string cur;
    bool in_string = false;
    bool escape = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (!in_string) {
            if (c == '#') {
                break; // comment
            } else if (c == '"') {
                if (!cur.empty()) { tokens.push_back(Token{cur, false}); cur.clear(); }
                in_string = true;
            } else if (std::isspace(static_cast<unsigned char>(c))) {
                if (!cur.empty()) { tokens.push_back(Token{cur, false}); cur.clear(); }
            } else {
                cur.push_back(c);
            }
        } else {
            if (escape) {
                if (c == '"' || c == '\\') cur.push_back(c);
                else cur.push_back(c);
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                // end string token
                tokens.push_back(Token{cur, true});
                cur.clear();
                in_string = false;
            } else {
                cur.push_back(c);
            }
        }
    }
    if (!in_string && !cur.empty()) tokens.push_back(Token{cur, false});
    // Unterminated string: treat whatever gathered as a quoted token for parser error detection
    if (in_string) {
        tokens.push_back(Token{cur, true});
    }
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

const std::vector<std::string>& ParserLogger::get_errors(){
    return error_message_vector;
}

const std::vector<std::string>& ParserLogger::get_debug(){
    return debug_message_vector;
}

bool ParserLogger::empty_err() const{
    return error_message_vector.empty();
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

bool parse_istream(std::istream& in, const std::string& source_name, World& out, ParserLogger& log) {
    std::string line;
    int line_no = 0;
    std::optional<Day> current_day;

    log.log_debug(__FUNCTION__, __LINE__, "don't panic");

    ChargeDirectiveParser cn_parser(source_name,out,log,current_day);
    TaskDirectiveParser task_parser(source_name,out,log,current_day);
    WeightDirectiveParser wt_parser(source_name,out,log,current_day);
    DayDirectiveParser day_parser(source_name,out,log,current_day);
    LogDirectiveParser log_parser(source_name,out,log,current_day);
    OptionDirectiveParser opt_parser(source_name,out,log,current_day);

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
    if (current_day.has_value() && 
        !current_day.value().logs.empty() && 
        current_day.value().logs.back().kind != LogKind::None)
    {
        return log.log_error(source_name, line_no, "file ended but last day not closed with final uncharged log");
    }

    return true;
}

bool parse_file(const std::string& path, World& out, ParserLogger& log) {
    std::ifstream in(path);
    if (!in) {
        log.log_error("parse_file", __LINE__, "unable to open file: " + path);
        return false;
    }
    return parse_istream(in, path, out, log);
}

DirectiveParser::DirectiveParser(
        const std::string& source_name,
        struct World& out,
        ParserLogger& log,
        std::optional<Day>& current_day):
source_name(source_name),
out(out),
log(log),
current_day(current_day)
{
}

ChargeDirectiveParser::ChargeDirectiveParser(
        const std::string& source_name,
        struct World& out,
        ParserLogger& log,
        std::optional<Day>& current_day)
    : DirectiveParser(source_name, out, log, current_day) {}

TaskDirectiveParser::TaskDirectiveParser(
        const std::string& source_name,
        struct World& out,
        ParserLogger& log,
        std::optional<Day>& current_day)
    : DirectiveParser(source_name, out, log, current_day) {}

WeightDirectiveParser::WeightDirectiveParser(
        const std::string& source_name,
        struct World& out,
        ParserLogger& log,
        std::optional<Day>& current_day)
    : DirectiveParser(source_name, out, log, current_day) {}

DayDirectiveParser::DayDirectiveParser(
        const std::string& source_name,
        struct World& out,
        ParserLogger& log,
        std::optional<Day>& current_day)
    : DirectiveParser(source_name, out, log, current_day) {}

LogDirectiveParser::LogDirectiveParser(
        const std::string& source_name,
        struct World& out,
        ParserLogger& log,
        std::optional<Day>& current_day)
    : DirectiveParser(source_name, out, log, current_day) {}

OptionDirectiveParser::OptionDirectiveParser(
        const std::string& source_name,
        struct World& out,
        ParserLogger& log,
        std::optional<Day>& current_day)
    : DirectiveParser(source_name, out, log, current_day) {}

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
    if (out.charges.find(id) != out.charges.end()) return log.log_error(source_name, line_no, "duplicate charge id: " + id);
    ChargeNumber cn;
    cn.id = id;
    cn.description = desc;
    cn.budget_ticks = budget_ticks;
    cn.priority = prio;
    out.charges.emplace(id, std::move(cn));
    return true;
}

bool TaskDirectiveParser::parse(const int line_no, const std::vector<Token>& tokens)
{
    if (tokens.size() < 3) return log.log_error(source_name, line_no, "task requires: task <name> \"<description>\"");
    const std::string name = tokens[1].text;
    if (!tokens[2].quoted) return log.log_error(source_name, line_no, "task description must be quoted");
    if (out.tasks.find(name) != out.tasks.end()) return log.log_error(source_name, line_no, "duplicate task: " + name);
    Task t;
    t.name = name;
    t.description = tokens[2].text;
    out.tasks.emplace(name, std::move(t));
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
    auto& task = out.tasks[task_name];
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
    if (!out.days.empty() && d.ord < out.days.back().date.ord) return log.log_error(source_name, line_no, "days out of order (must be non-decreasing)");
    Day day;
    day.date = d;
    out.days.push_back(std::move(day));
    current_day = out.days.back();
    return true;
}

bool LogDirectiveParser::parse(const int line_no, const std::vector<Token>& tokens)
{
    if (!current_day.has_value()) return log.log_error(source_name, line_no, "log before any day declared");
    if (tokens.size() < 2) return log.log_error(source_name, line_no, "log requires: log <time> [<task_or_charge>] [\"<description>\"]");
    double th;
    if (!try_parse_double(tokens[1].text, th)) return log.log_error(source_name, line_no, "invalid time in log");
    Tick t = hours_to_ticks(th);
    if (!current_day->logs.empty() && !(current_day->logs.back().time_ticks < t)) return log.log_error(source_name, line_no, "non-increasing log time");
    Log lg;
    lg.time_ticks = t;
    lg.line_no = line_no;
    if (tokens.size() >= 3 && !tokens[2].quoted) {
        lg.ref_id = tokens[2].text;
        // classify later in validation or attempt now
        if (out.tasks.find(lg.ref_id) != out.tasks.end()) lg.kind = LogKind::Task;
        else if (out.charges.find(lg.ref_id) != out.charges.end()) lg.kind = LogKind::Charge;
        else lg.kind = LogKind::Task; // default assume task; validator will catch if unknown
        if (tokens.size() >= 4) {
            if (!tokens[3].quoted) return log.log_error(source_name, line_no, "log description must be quoted if present");
            lg.description = tokens[3].text;
        }
    } else {
        lg.kind = LogKind::None;
    }
    current_day.value().logs.push_back(std::move(lg));
    return true;
}

bool OptionDirectiveParser::parse(const int line_no, const std::vector<Token>& tokens)
{
    if (tokens.size() < 3) return log.log_error(source_name, line_no, "opt requires: opt <key> <value>");
    const std::string opt_key = tokens[1].text;
    const std::string opt_value = tokens[2].text;
    static const std::string valid_keys = {"rounding"};
    return true;
}

bool DirectiveParser::ensure_day_closed(const int lno) {
    if (!current_day.has_value()) return true;
    if (current_day.value().logs.empty()) return true;
    if (current_day.value().logs.back().kind != LogKind::None) {
        return log.log_error(source_name, lno, "day started before previous day closed (missing final uncharged log)");
    }
    return true;
}

} // namespace ottr
