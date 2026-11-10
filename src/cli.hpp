#pragma once
#include <string>
#include <vector>
#include "model.hpp"

namespace ottr {

// Parsed command-line arguments.
struct CliOptions {
    std::string path;   // timecard source file
    DateFilter filter;  // optional single-day or range filter
};

enum class CliStatus {
    Ok,     // options parsed; run the report
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
