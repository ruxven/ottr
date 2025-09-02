#include "util.hpp"
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace ottr {

bool parse_mmdd(const std::string& s, Date& out) {
    // Expect MM/DD with 1 or 2 digits for month/day
    int mm = 0, dd = 0;
    size_t slash = s.find('/');
    if (slash == std::string::npos) return false;
    std::string a = s.substr(0, slash);
    std::string b = s.substr(slash + 1);
    if (a.empty() || b.empty()) return false;
    for (char c : a) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    for (char c : b) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    mm = std::atoi(a.c_str());
    dd = std::atoi(b.c_str());
    if (mm < 1 || mm > 12) return false;
    if (dd < 1 || dd > 31) return false;
    out.raw = s;
    out.ord = mm * 100 + dd;
    return true;
}

bool try_parse_double(const std::string& s, double& out) {
    char* end = nullptr;
    out = std::strtod(s.c_str(), &end);
    return end && *end == '\0';
}

bool try_parse_ll(const std::string& s, long long& out) {
    char* end = nullptr;
    out = std::strtoll(s.c_str(), &end, 10);
    return end && *end == '\0';
}

} // namespace ottr
