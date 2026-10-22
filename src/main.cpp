#include <cmath>
#include <string>
#include <vector>
#include "parser.hpp"
#include "validate.hpp"
#include "engine.hpp"
#include "report.hpp"
#include "util.hpp"
#include <cstdlib>
#include <iomanip>
#include <iostream>

using namespace ottr;

namespace {

void print_usage(const char* prog) {
    std::cerr << "Usage:\n"
                 "  " << prog << " FILE\n"
                 "  " << prog << " FILE START\n"
                 "  " << prog << " FILE START END\n";
}

// Returns 0 on success, or the process exit code.
int parse_args(int argc, char** argv, std::string& path, DateFilter& filter) {
    if (argc < 2 || argc > 4) {
        print_usage(argv[0]);
        return 2;
    }

    path = argv[1];
    if (argc >= 3) {
        Date start;
        if (!parse_mmdd(argv[2], start)) {
            std::cerr << "Invalid START date (MM/DD)\n";
            return 2;
        }
        filter.has_start = true;
        filter.start = start;
    }
    if (argc == 4) {
        Date end;
        if (!parse_mmdd(argv[3], end)) {
            std::cerr << "Invalid END date (MM/DD)\n";
            return 2;
        }
        filter.has_end = true;
        filter.end = end;
    }
    return 0;
}

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
    std::cerr << "\nAudit:\n";
    for (const AuditEntry& entry : agg.audit) {
        std::cerr << entry.date.raw << " " << entry.note << " " << entry.source
                  << " -> " << entry.charge_id << " "
                  << std::fixed << std::setprecision(1)
                  << (static_cast<double>(entry.ticks) / 10.0) << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string path;
    DateFilter filter;
    const int arg_status = parse_args(argc, argv, path, filter);
    if (arg_status != 0) return arg_status;

    World world;
    Aggregation agg;
    if (!run(path, filter, world, agg)) return 1;

    std::cout << render_full_table(world, agg);
    print_single_day(filter, world, agg);
    print_audit(world, agg);
    return 0;
}
