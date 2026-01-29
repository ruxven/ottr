#pragma once
#include <string>
#include "model.hpp"

namespace ottr {

class Validator {
public:
    Validator() = default;

    bool log_error(const std::string& path, const int line_no, const std::string& msg);
    bool log_debug(const std::string& path, const int line_no, const std::string& msg);

    const std::vector<std::string>& get_errors();
    const std::vector<std::string>& get_debug();
    bool empty_err() const;
    bool find_err(const std::string&) const;

    // Returns true on OK, otherwise false.
    bool validate_world(const std::string& file, const World& world);
private:
    std::vector<std::string> error_message_vector;
    std::vector<std::string> debug_message_vector;
};


} // namespace ottr
