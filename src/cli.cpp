#include "cli.hpp"
#include "util.hpp"
#include <sstream>

namespace ottr {

std::string help_text(const std::string& prog) {
    std::ostringstream oss;
    oss << "ottr - One Tenth Time Reporter\n"
        << "\n"
        << "Usage:\n"
        << "  " << prog << " FILE\n"
        << "  " << prog << " FILE START\n"
        << "  " << prog << " FILE START END\n"
        << "  " << prog << " --help\n"
        << "\n"
        << "Run modes:\n"
        << "  FILE            Report every day in the timecard.\n"
        << "  FILE START      Report the full table filtered to START, plus a\n"
        << "                  compact single-day table for quick copy/paste.\n"
        << "  FILE START END  Report every day from START through END, inclusive.\n"
        << "\n"
        << "Arguments:\n"
        << "  FILE   Path to the timecard source file.\n"
        << "  START  First day to include, in MM/DD format.\n"
        << "  END    Last day to include, in MM/DD format.\n"
        << "\n"
        << "Options:\n"
        << "  -h, --help  Show this help and exit.\n"
        << "  --          Treat all following arguments as positional.\n"
        << "\n"
        << "Calculation options are set inside the timecard with 'opt'\n"
        << "directives. See:\n"
        << "  https://github.com/ruxven/ottr/blob/main/"
           "README.md#configuration-options\n";
    return oss.str();
}

CliStatus parse_cli(const std::vector<std::string>& args, CliOptions& out,
                    std::string& error) {
    std::vector<std::string> positional;
    bool end_of_options = false;

    for (const std::string& arg : args) {
        if (!end_of_options && arg == "--") {
            end_of_options = true;
            continue;
        }
        if (!end_of_options && (arg == "-h" || arg == "--help")) {
            return CliStatus::Help;
        }
        if (!end_of_options && arg.size() > 1 && arg[0] == '-') {
            error = "unknown option: '" + arg + "'";
            return CliStatus::Error;
        }
        positional.push_back(arg);
    }

    if (positional.empty()) {
        error = "missing FILE argument";
        return CliStatus::Error;
    }
    if (positional.size() > 3) {
        error = "too many arguments (expected FILE [START [END]])";
        return CliStatus::Error;
    }

    out = CliOptions{};
    out.path = positional[0];

    if (positional.size() >= 2) {
        Date start;
        if (!parse_mmdd(positional[1], start)) {
            error = "invalid START date '" + positional[1] + "' (expected MM/DD)";
            return CliStatus::Error;
        }
        out.filter.has_start = true;
        out.filter.start = start;
    }
    if (positional.size() == 3) {
        Date end;
        if (!parse_mmdd(positional[2], end)) {
            error = "invalid END date '" + positional[2] + "' (expected MM/DD)";
            return CliStatus::Error;
        }
        out.filter.has_end = true;
        out.filter.end = end;
    }
    return CliStatus::Ok;
}

} // namespace ottr
