#include "parser.hpp"
#include "model.hpp"
#include "util.hpp"
#include <fstream>
#include <sstream>

namespace ottr {

#include <cctype>

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

static bool parse_error(const std::string& path, int line_no, const std::string& msg, std::string& out_err) {
    std::ostringstream oss;
    oss << path << ":" << line_no << ": " << msg;
    out_err = oss.str();
    return false;
}

bool parse_file(const std::string& path, World& out, std::string& error_message) {
    std::ifstream in(path);
    if (!in) {
        error_message = "unable to open file: " + path;
        return false;
    }
    std::string line;
    int line_no = 0;
    Day* current_day = nullptr;

    auto ensure_day_closed = [&](int lno) -> bool {
        if (!current_day) return true;
        if (current_day->logs.empty()) return true;
        if (current_day->logs.back().kind != LogKind::None) {
            return parse_error(path, lno, "day started before previous day closed (missing final uncharged log)", error_message);
        }
        return true;
    };

    while (std::getline(in, line)) {
        ++line_no;
        auto tokens = tokenize_line(line);
        if (tokens.empty()) continue;
        const std::string& head = tokens[0].text;
        if (head == "cn") {
            if (tokens.size() < 4) return parse_error(path, line_no, "cn requires: cn <id> \"<description>\" <hours> [<priority>]", error_message);
            const std::string id = tokens[1].text;
            if (!tokens[2].quoted) return parse_error(path, line_no, "cn description must be quoted", error_message);
            const std::string desc = tokens[2].text;
            double hours;
            if (!try_parse_double(tokens[3].text, hours)) return parse_error(path, line_no, "invalid hours in cn", error_message);
            long long budget_ticks = hours_to_ticks(hours);
            int prio = 0;
            if (tokens.size() >= 5) {
                long long tmp;
                if (!try_parse_ll(tokens[4].text, tmp)) return parse_error(path, line_no, "invalid priority in cn", error_message);
                prio = static_cast<int>(tmp);
            }
            if (out.charges.find(id) != out.charges.end()) return parse_error(path, line_no, "duplicate charge id: " + id, error_message);
            ChargeNumber cn;
            cn.id = id;
            cn.description = desc;
            cn.budget_ticks = budget_ticks;
            cn.priority = prio;
            out.charges.emplace(id, std::move(cn));
        } else if (head == "task") {
            if (tokens.size() < 3) return parse_error(path, line_no, "task requires: task <name> \"<description>\"", error_message);
            const std::string name = tokens[1].text;
            if (!tokens[2].quoted) return parse_error(path, line_no, "task description must be quoted", error_message);
            if (out.tasks.find(name) != out.tasks.end()) return parse_error(path, line_no, "duplicate task: " + name, error_message);
            Task t;
            t.name = name;
            t.description = tokens[2].text;
            out.tasks.emplace(name, std::move(t));
        } else if (head == "wt") {
            if (tokens.size() < 4) return parse_error(path, line_no, "wt requires: wt <task_name> <charge_id> <weight>", error_message);
            const std::string task_name = tokens[1].text;
            const std::string charge_id = tokens[2].text;
            long long wll;
            if (!try_parse_ll(tokens[3].text, wll) || wll <= 0) return parse_error(path, line_no, "invalid weight in wt", error_message);
            // Allow forward references; validation will check existence.
            auto& task = out.tasks[task_name];
            if (task.name.empty()) task.name = task_name; // placeholder if undeclared
            task.weights[charge_id] += static_cast<int>(wll);
        } else if (head == "day") {
            if (tokens.size() < 2) return parse_error(path, line_no, "day requires: day <MM/DD>", error_message);
            if (!ensure_day_closed(line_no)) return false;
            Date d;
            if (!parse_mmdd(tokens[1].text, d)) return parse_error(path, line_no, "invalid date MM/DD", error_message);
            // non-decreasing order
            if (!out.days.empty() && d.ord < out.days.back().date.ord) return parse_error(path, line_no, "days out of order (must be non-decreasing)", error_message);
            Day day;
            day.date = d;
            out.days.push_back(std::move(day));
            current_day = &out.days.back();
        } else if (head == "log") {
            if (!current_day) return parse_error(path, line_no, "log before any day declared", error_message);
            if (tokens.size() < 2) return parse_error(path, line_no, "log requires: log <time> [<task_or_charge>] [\"<description>\"]", error_message);
            double th;
            if (!try_parse_double(tokens[1].text, th)) return parse_error(path, line_no, "invalid time in log", error_message);
            Tick t = hours_to_ticks(th);
            if (!current_day->logs.empty() && !(current_day->logs.back().time_ticks < t)) return parse_error(path, line_no, "non-increasing log time", error_message);
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
                    if (!tokens[3].quoted) return parse_error(path, line_no, "log description must be quoted if present", error_message);
                    lg.description = tokens[3].text;
                }
            } else {
                lg.kind = LogKind::None;
            }
            current_day->logs.push_back(std::move(lg));
        } else {
            return parse_error(path, line_no, "unknown directive: " + head, error_message);
        }
    }

    // Ensure final day is closed
    if (current_day && !current_day->logs.empty() && current_day->logs.back().kind != LogKind::None) {
        return parse_error(path, line_no, "file ended but last day not closed with final uncharged log", error_message);
    }

    return true;
}

} // namespace ottr
