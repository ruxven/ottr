#include "validate.hpp"
#include "model.hpp"
#include <sstream>

namespace ottr {

static void append_err(std::ostringstream& oss, const std::string& file, int line, const std::string& msg) {
    if (oss.tellp() > 0) oss << '\n';
    oss << file << ":" << line << ": " << msg;
}

std::string validate_world(const std::string& file, const World& world) {
    std::ostringstream errs;

    // Check tasks exist for all referenced weights and charges exist too
    for (const auto& [tname, t] : world.tasks) {
        for (const auto& [cid, w] : t.weights) {
            if (!world.charges.count(cid)) {
                append_err(errs, file, 0, "wt references unknown charge: " + cid + " (task: " + tname + ")");
            }
        }
    }

    // For each day, ensure >= 2 logs and last is None (parser ensures closing but double-check)
    for (const auto& day : world.days) {
        if (day.logs.size() < 2) {
            append_err(errs, file, 0, "day " + day.date.raw + " must contain at least two log entries");
        }
        if (!day.logs.empty() && day.logs.back().kind != LogKind::None) {
            append_err(errs, file, 0, "day " + day.date.raw + " not closed with final uncharged log");
        }
        // Validate log references
        for (const auto& lg : day.logs) {
            if (lg.kind == LogKind::Task) {
                bool known_task = world.tasks.count(lg.ref_id) > 0;
                bool is_charge = world.charges.count(lg.ref_id) > 0;
                bool is_nc = (lg.ref_id == "nc");
                if (!known_task && !is_charge && !is_nc) {
                    append_err(errs, file, lg.line_no, "log references unknown task or charge: " + lg.ref_id);
                }
            } else if (lg.kind == LogKind::Charge) {
                if (!world.charges.count(lg.ref_id)) {
                    append_err(errs, file, lg.line_no, "log references unknown charge: " + lg.ref_id);
                }
            }
        }
    }

    return errs.str();
}

} // namespace ottr
