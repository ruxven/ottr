#pragma once
#include <string>
#include "model.hpp"

namespace ottr {

// Returns empty string on OK, otherwise error message.
std::string validate_world(const std::string& file, const World& world);

} // namespace ottr
