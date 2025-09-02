#include "aggregator.hpp"

namespace ottr {

void Aggregator::add(const Date& d, const std::string& charge_id, Tick ticks) {
    if (ticks <= 0) return;
    snapshot_.by_charge_by_day[charge_id][d.raw] += ticks;
    snapshot_.totals_by_charge[charge_id] += ticks;
    snapshot_.totals_by_day[d.raw] += ticks;
}

Tick Aggregator::total_for_charge(const std::string& charge_id) const {
    auto it = snapshot_.totals_by_charge.find(charge_id);
    return it == snapshot_.totals_by_charge.end() ? 0 : it->second;
}

Tick Aggregator::by_day_for_charge(const std::string& charge_id, const std::string& date_raw) const {
    auto it = snapshot_.by_charge_by_day.find(charge_id);
    if (it == snapshot_.by_charge_by_day.end()) return 0;
    auto it2 = it->second.find(date_raw);
    return it2 == it->second.end() ? 0 : it2->second;
}

Tick Aggregator::total_for_day(const std::string& date_raw) const {
    auto it = snapshot_.totals_by_day.find(date_raw);
    return it == snapshot_.totals_by_day.end() ? 0 : it->second;
}

void Aggregator::set_dates(std::vector<Date> d) {
    dates_in_order_ = std::move(d);
    snapshot_.dates_in_order = dates_in_order_;
}

} // namespace ottr
