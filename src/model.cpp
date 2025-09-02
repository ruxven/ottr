#include "model.hpp"

namespace ottr {

bool DateFilter::includes(const Date& d) const {
    if (!has_start && !has_end) return true;
    if (has_start && !has_end) return d.ord == start.ord;
    if (!has_start && has_end) return d.ord <= end.ord; // not used by CLI but keep for completeness
    return d.ord >= start.ord && d.ord <= end.ord;
}

} // namespace ottr
