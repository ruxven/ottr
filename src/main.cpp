#include <cmath>
#include <string>
#include <vector>
#include "parser.hpp"
#include "validate.hpp"
#include "engine.hpp"
#include "report.hpp"
#include "util.hpp"
#include <iostream>

using namespace ottr;

static void print_usage(const char* prog) {
    std::cerr << "Usage:\n"
                 "  " << prog << " FILE\n"
                 "  " << prog << " FILE START\n"
                 "  " << prog << " FILE START END\n";
}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        print_usage(argv[0]);
        return 2;
    }

    system("echo \"Processing started\"");

    std::string path = argv[1];
    DateFilter filter;

    if (argc >= 3) {
        Date d1; if (!parse_mmdd(argv[2], d1)) { std::cerr << "Invalid START date (MM/DD)\n"; return 2; }
        filter.has_start = true; filter.start = d1;
    }
    if (argc == 4) {
        Date d2; if (!parse_mmdd(argv[3], d2)) { std::cerr << "Invalid END date (MM/DD)\n"; return 2; }
        filter.has_end = true; filter.end = d2;
    }

    World world;
    std::string err;
    if (!parse_file(path, world, err)) {
        std::cerr << err << "\n";
        return 1;
    }

    std::string verr = validate_world(path, world);
    if (!verr.empty()) {
        std::cerr << verr << "\n";
        return 1;
    }

    Aggregation agg;
    EngineOptions opts; opts.filter = filter;
    if (!process_world(world, opts, agg, err)) {
        std::cerr << err << "\n";
        return 1;
    }

    // Always render full table
    std::cout << render_full_table(world, agg);

    // If single-day filter, render compact table
    if (filter.has_start && !filter.has_end) {
        // Find the date object in agg.dates_in_order that matches start
        for (const auto& d : agg.dates_in_order) {
            if (d.ord == filter.start.ord) {
                std::cout << "\n" << render_single_day_table(world, agg, d);
                break;
            }
        }
    }

    return 0;
}
