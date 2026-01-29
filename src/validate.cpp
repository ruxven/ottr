#include "validate.hpp"
#include "model.hpp"
#include <sstream>

namespace ottr {

bool Validator::validate_world(const std::string& file, const World& world) {
    bool valid = true;

    // Check tasks exist for all referenced weights and charges exist too
    for (const auto& [tname, t] : world.tasks) {
        for (const auto& [cid, w] : t.weights) {
            if (!world.charges.count(cid)) {
                log_error(file, 0, "wt references unknown charge: " + cid + " (task: " + tname + ")");
		valid = false;
            }
        }
    }

    // For each day, ensure >= 2 logs and last is None (parser ensures closing but double-check)
    for (const auto& day : world.days) {
        if (day.logs.size() < 2) {
            log_error(file, 0, "day " + day.date.raw + " must contain at least two log entries");
            valid = false;
        }
        if (!day.logs.empty() && day.logs.back().kind != LogKind::None) {
            log_error(file, 0, "day " + day.date.raw + " not closed with final uncharged log");
            valid = false;
        }
        // Validate log references
        for (const auto& lg : day.logs) {
            if (lg.kind == LogKind::Task) {
                bool known_task = world.tasks.count(lg.ref_id) > 0;
                bool is_charge = world.charges.count(lg.ref_id) > 0;
                bool is_nc = (lg.ref_id == "nc");
                if (!known_task && !is_charge && !is_nc) {
                    log_error(file, lg.line_no, "log references unknown task or charge: " + lg.ref_id);
                    valid = false;
                }
            } else if (lg.kind == LogKind::Charge) {
                if (!world.charges.count(lg.ref_id)) {
                    log_error(file, lg.line_no, "log references unknown charge: " + lg.ref_id);
                    valid = false;
                }
            }
        }
    }

    return valid;
}

bool Validator::log_debug(const std::string& path, const int line_no, const std::string& msg) {
    std::ostringstream oss;
    oss << path << ":" << line_no << ": " << msg;
    debug_message_vector.push_back(oss.str());
    return false;
}
bool Validator::log_error(const std::string& path, const int line_no, const std::string& msg) {
    std::ostringstream oss;
    oss << path << ":" << line_no << ": " << msg;
    error_message_vector.push_back(oss.str());
    return false;
}

const std::vector<std::string>& Validator::get_errors(){
    return error_message_vector;
}

const std::vector<std::string>& Validator::get_debug(){
    return debug_message_vector;
}

bool Validator::empty_err() const{
    return error_message_vector.empty();
}

bool Validator::find_err(const std::string& q) const
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

} // namespace ottr
