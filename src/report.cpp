#include "report.hpp"
#include <algorithm>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>

namespace ottr {

static std::vector<std::string> sorted_charge_ids(const World& world) {
    std::vector<std::string> ids;
    ids.reserve(world.charges.size());
    for (const auto& kv : world.charges) ids.push_back(kv.first);
    std::sort(ids.begin(), ids.end());
    return ids;
}

static std::string ticks_to_fixed(Tick t) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed); oss << std::setprecision(1) << (static_cast<double>(t) / 10.0);
    return oss.str();
}

static void compute_widths(const World& world, const Aggregation& agg, std::map<std::string, size_t>& colw, std::vector<std::string>& headers) {
    headers.clear();
    headers.push_back("Charge");
    for (const auto& d : agg.dates_in_order) headers.push_back(d.raw);
    headers.push_back("Total");
    headers.push_back("Rem");
    for (const auto& h : headers) colw[h] = std::max(colw[h], h.size());

    auto ids = sorted_charge_ids(world);
    for (const auto& cid : ids) {
        colw["Charge"] = std::max(colw["Charge"], cid.size());
        Tick total = 0;
        for (const auto& d : agg.dates_in_order) {
            Tick v = 0;
            auto it1 = agg.by_charge_by_day.find(cid);
            if (it1 != agg.by_charge_by_day.end()) {
                auto it2 = it1->second.find(d.raw);
                if (it2 != it1->second.end()) v = it2->second;
            }
            auto s = ticks_to_fixed(v);
            colw[d.raw] = std::max(colw[d.raw], s.size());
            total += v;
        }
        auto st = ticks_to_fixed(total);
        colw["Total"] = std::max(colw["Total"], st.size());
        // remaining budget length depends on budget
        auto cit = world.charges.find(cid);
        if (cit != world.charges.end()) {
            Tick rem = cit->second.budget_ticks - total;
            auto sr = ticks_to_fixed(rem);
            colw["Rem"] = std::max(colw["Rem"], sr.size());
        }
    }

    // Total row widths
    for (const auto& d : agg.dates_in_order) {
        auto s = ticks_to_fixed(agg.totals_by_day.count(d.raw) ? agg.totals_by_day.at(d.raw) : 0);
        colw[d.raw] = std::max(colw[d.raw], s.size());
    }
    auto grand = 0ll;
    for (const auto& kv : agg.totals_by_charge) grand += kv.second;
    colw["Total"] = std::max(colw["Total"], ticks_to_fixed(grand).size());
}

static void append_row(std::ostringstream& oss, const std::vector<std::pair<std::string, std::string>>& cols, const std::map<std::string, size_t>& colw) {
    oss << "|";
    for (const auto& c : cols) {
        const auto& head = c.first;
        const auto& val = c.second;
        size_t w = colw.at(head);
        bool right = (head != "Charge");
        oss << " " << (right ? std::string(w - val.size(), ' ') + val : val + std::string(w - val.size(), ' ')) << " |";
    }
    oss << "\n";
}

std::string render_full_table(const World& world, const Aggregation& agg) {
    std::ostringstream oss;
    std::map<std::string, size_t> colw;
    std::vector<std::string> headers;
    compute_widths(world, agg, colw, headers);
    // Header
    std::vector<std::pair<std::string, std::string>> hcols;
    for (const auto& h : headers) hcols.push_back({h, h});
    append_row(oss, hcols, colw);

    auto ids = sorted_charge_ids(world);
    for (const auto& cid : ids) {
        Tick total = 0;
        std::vector<std::pair<std::string, std::string>> cols;
        cols.push_back({"Charge", cid});
        for (const auto& d : agg.dates_in_order) {
            Tick v = 0;
            auto it1 = agg.by_charge_by_day.find(cid);
            if (it1 != agg.by_charge_by_day.end()) {
                auto it2 = it1->second.find(d.raw);
                if (it2 != it1->second.end()) v = it2->second;
            }
            cols.push_back({d.raw, ticks_to_fixed(v)});
            total += v;
        }
        cols.push_back({"Total", ticks_to_fixed(total)});
        auto cit = world.charges.find(cid);
        if (cit != world.charges.end()) {
            Tick rem = cit->second.budget_ticks - total;
            cols.push_back({"Rem", ticks_to_fixed(rem)});
        } else {
            cols.push_back({"Rem", ""});
        }
        append_row(oss, cols, colw);
    }

    // Total row
    std::vector<std::pair<std::string, std::string>> tcols;
    tcols.push_back({"Charge", "Total"});
    Tick grand = 0;
    for (const auto& d : agg.dates_in_order) {
        Tick s = agg.totals_by_day.count(d.raw) ? agg.totals_by_day.at(d.raw) : 0;
        tcols.push_back({d.raw, ticks_to_fixed(s)});
        grand += s;
    }
    tcols.push_back({"Total", ticks_to_fixed(grand)});
    tcols.push_back({"Rem", ""});
    append_row(oss, tcols, colw);

    return oss.str();
}

std::string render_single_day_table(const World& world, const Aggregation& agg, const Date& day) {
    std::ostringstream oss;
    std::map<std::string, size_t> colw;
    colw["Charge"] = std::string("Charge").size();
    colw[day.raw] = day.raw.size();
    // Measure widths
    auto ids = sorted_charge_ids(world);
    Tick daysum = 0;
    for (const auto& cid : ids) {
        colw["Charge"] = std::max(colw["Charge"], cid.size());
        Tick v = 0;
        auto it1 = agg.by_charge_by_day.find(cid);
        if (it1 != agg.by_charge_by_day.end()) {
            auto it2 = it1->second.find(day.raw);
            if (it2 != it1->second.end()) v = it2->second;
        }
        daysum += v;
        colw[day.raw] = std::max(colw[day.raw], ticks_to_fixed(v).size());
    }

    // Header
    append_row(oss, {{"Charge", "Charge"}, {day.raw, day.raw}}, colw);
    // Rows
    for (const auto& cid : ids) {
        Tick v = 0;
        auto it1 = agg.by_charge_by_day.find(cid);
        if (it1 != agg.by_charge_by_day.end()) {
            auto it2 = it1->second.find(day.raw);
            if (it2 != it1->second.end()) v = it2->second;
        }
        append_row(oss, {{"Charge", cid}, {day.raw, ticks_to_fixed(v)}}, colw);
    }
    // Total row
    append_row(oss, {{"Charge", "Total"}, {day.raw, ticks_to_fixed(daysum)}}, colw);

    return oss.str();
}

} // namespace ottr
