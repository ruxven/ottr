#include "validate.hpp"
#include "model.hpp"
#include <sstream>

namespace ottr {

namespace {

bool validate_spill_target(const std::string& file, const World& world,
                           Validator& validator) {
    if (world.options.budget != BudgetPolicy::Spill) return true;
    if (world.charges.count(world.options.overflow_charge) > 0) return true;
    validator.log_error(file, 0, "overflow_charge references unknown charge: " +
                                     world.options.overflow_charge);
    return false;
}

bool validate_task_weights(const std::string& file, const World& world,
                           Validator& validator) {
    bool valid = true;
    for (const auto& [tname, task] : world.tasks) {
        for (const auto& [cid, weight] : task.weights) {
            if (world.charges.count(cid) > 0) continue;
            validator.log_error(file, 0, "wt references unknown charge: " + cid +
                                             " (task: " + tname + ")");
            valid = false;
        }
    }
    return valid;
}

bool validate_log_ref(const std::string& file, const World& world, const Log& entry,
                      Validator& validator) {
    if (entry.kind == LogKind::Task) {
        const bool known_task = world.tasks.count(entry.ref_id) > 0;
        const bool is_charge = world.charges.count(entry.ref_id) > 0;
        const bool is_nc = (entry.ref_id == "nc");
        if (known_task || is_charge || is_nc) return true;
        validator.log_error(file, entry.line_no,
                            "log references unknown task or charge: " + entry.ref_id);
        return false;
    }
    if (entry.kind == LogKind::Charge && world.charges.count(entry.ref_id) == 0) {
        validator.log_error(file, entry.line_no,
                            "log references unknown charge: " + entry.ref_id);
        return false;
    }
    return true;
}

// A day needs at least two logs and must end on the closing uncharged log.
bool validate_day(const std::string& file, const World& world, const Day& day,
                  Validator& validator) {
    bool valid = true;
    if (day.logs.size() < 2) {
        validator.log_error(file, 0, "day " + day.date.raw +
                                         " must contain at least two log entries");
        valid = false;
    }
    if (!day.logs.empty() && day.logs.back().kind != LogKind::None) {
        validator.log_error(file, 0, "day " + day.date.raw +
                                         " not closed with final uncharged log");
        valid = false;
    }
    for (const Log& entry : day.logs) {
        if (!validate_log_ref(file, world, entry, validator)) valid = false;
    }
    return valid;
}

} // namespace

bool Validator::validate_world(const std::string& file, const World& world) {
    bool valid = true;
    if (!validate_spill_target(file, world, *this)) valid = false;
    if (!validate_task_weights(file, world, *this)) valid = false;
    for (const Day& day : world.days) {
        if (!validate_day(file, world, day, *this)) valid = false;
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
