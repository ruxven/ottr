#include <string>
#include <vector>
#include "cli.hpp"
#include "generate.hpp"
#include "parser.hpp"
#include "validate.hpp"
#include "engine.hpp"
#include "report.hpp"
#include <cstdlib>
#include <iostream>

using namespace ottr;

namespace {

void print_lines(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        std::cerr << line << "\n";
    }
}

void print_debug_if_enabled(ParserLogger& log) {
    const char* debug_env = std::getenv("OTTR_DEBUG");
    if (debug_env == nullptr || std::string(debug_env) != "ON") return;
    print_lines(log.get_debug());
}

// Parse, validate, and process the timecard. Diagnostics go to stderr.
bool run(const std::string& path, const DateFilter& filter, World& world,
         Aggregation& agg) {
    ParserLogger log;
    const bool parse_ok = parse_file(path, world, log);
    print_debug_if_enabled(log);
    print_lines(log.get_warnings());
    if (!parse_ok) {
        print_lines(log.get_errors());
        return false;
    }

    std::string opt_error;
    if (!world.options.validate(opt_error)) {
        std::cerr << path << ": " << opt_error << "\n";
        return false;
    }

    Validator validator;
    if (!validator.validate_world(path, world)) {
        print_lines(validator.get_errors());
        return false;
    }

    EngineOptions opts;
    opts.filter = filter;
    opts.calc_options = world.options;
    std::string error_message;
    std::vector<std::string> warnings;
    if (!process_world(world, opts, agg, error_message, &warnings)) {
        std::cerr << error_message << "\n";
        return false;
    }
    print_lines(warnings);
    return true;
}

void print_single_day(const DateFilter& filter, const World& world,
                      const Aggregation& agg) {
    if (!filter.has_start || filter.has_end) return;
    for (const Date& date : agg.dates_in_order) {
        if (date.ord == filter.start.ord) {
            std::cout << "\n" << render_single_day_table(world, agg, date);
            return;
        }
    }
}

// Audit goes to stderr so the table stays clean when piped.
void print_audit(const World& world, const Aggregation& agg) {
    if (!world.options.audit) return;
    std::cerr << render_audit(agg);
}

} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(argc > 0 ? argc - 1 : 0));
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    CliOptions cli;
    std::string cli_error;
    const CliStatus status = parse_cli(args, cli, cli_error);
    if (status == CliStatus::Help) {
        std::cout << help_text(argv[0]);
        return 0;
    }
    if (status == CliStatus::Error) {
        std::cerr << "ottr: " << cli_error << "\n"
                  << "Try '" << argv[0] << " --help' for more information.\n";
        return 2;
    }

    if (cli.mode == CliMode::Generate) {
        CalendarDate start;
        CalendarDate end;
        std::string range_error;
        if (!parse_generate_range(cli.generate_start, cli.generate_end,
                                  current_year(), start, end, range_error)) {
            std::cerr << "ottr: " << range_error << "\n"
                      << "Try '" << argv[0] << " --help' for more information.\n";
            return 2;
        }
        std::cout << generate_timecard(start, end);
        return 0;
    }

    World world;
    Aggregation agg;
    if (!run(cli.path, cli.filter, world, agg)) return 1;

    std::cout << render_full_table(world, agg);
    print_single_day(cli.filter, world, agg);
    print_audit(world, agg);
    return 0;
}
