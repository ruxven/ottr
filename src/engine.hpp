#pragma once
#include "model.hpp"
#include <string>

namespace ottr {

struct EngineOptions {
    DateFilter filter; // date inclusion filter
};

// Process the world into aggregation according to SPEC/DESIGN.
// Returns false on error (currently none expected here), setting error_message.
bool process_world(const World& world, const EngineOptions& opts, Aggregation& out, std::string& error_message);

} // namespace ottr
