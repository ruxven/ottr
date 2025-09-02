#pragma once
#include "model.hpp"
#include <string>
#include <vector>

namespace ottr {

class Aggregator {
public:
    void add(const Date& d, const std::string& charge_id, Tick ticks);
    Tick total_for_charge(const std::string& charge_id) const;
    Tick by_day_for_charge(const std::string& charge_id, const std::string& date_raw) const;
    Tick total_for_day(const std::string& date_raw) const;
    const std::vector<Date>& dates() const { return dates_in_order_; }
    void set_dates(std::vector<Date> d);
    const Aggregation& snapshot() const { return snapshot_; }
private:
    Aggregation snapshot_;
    std::vector<Date> dates_in_order_;
};

} // namespace ottr
