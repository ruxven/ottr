#pragma once
#include <string>
#include <vector>
#include "model.hpp"

namespace ottr {

enum class CliMode {
    Report,   // read FILE and render a report
    Generate  // write a timecard skeleton for a date range
};

// Parsed command-line arguments.
struct CliOptions {
    CliMode mode = CliMode::Report;
    std::string path;   // timecard source file (Report mode)
    DateFilter filter;  // optional single-day or range filter (Report mode)
    std::string generate_start;  // raw START text (Generate mode)
    std::string generate_end;    // raw END text (Generate mode)
};

enum class CliStatus {
    Ok,     // options parsed; run the requested mode
    Help,   // caller should print help_text and exit 0
    Error   // caller should print `error` and exit non-zero
};

// Parse command-line arguments (excluding the program name) without doing any
// I/O, so the parser can be tested directly.
CliStatus parse_cli(const std::vector<std::string>& args, CliOptions& out,
                    std::string& error);

// Full help text for --help. `prog` is the program name (argv[0]).
std::string help_text(const std::string& prog);

} // namespace ottr
