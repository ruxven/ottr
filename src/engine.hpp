#pragma once
#include "model.hpp"
#include "options.hpp"
#include <string>
#include <vector>

namespace ottr {

struct EngineOptions {
    DateFilter filter;                    // date inclusion filter
    CalculationOptions calc_options;      // calculation configuration
};

// Process the world into aggregation according to SPEC/DESIGN.
// Returns false on error (currently none expected here), setting error_message.
// Recoverable findings (budget overruns under opt budget warn) are appended to
// warnings when a vector is supplied.
bool process_world(const World& world, const EngineOptions& opts, Aggregation& out,
                   std::string& error_message,
                   std::vector<std::string>* warnings = nullptr);

} // namespace ottr
