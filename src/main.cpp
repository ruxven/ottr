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
    ParserLogger log;
    bool parse_file_ok = parse_file(path, world, log);
    // if debugging enabled
    {
        const auto& dbg_vec = log.get_debug();
        for (size_t i = 0; i < dbg_vec.size(); i++)
        {
            const std::string& dbg_str = dbg_vec.at(i);
            std::cerr << dbg_str << "\n";
        }
    }
    if (!parse_file_ok) {
        const auto& err_vec = log.get_errors();
        for (size_t i = 0; i < err_vec.size(); i++)
        {
            const std::string& err_str = err_vec.at(i);
            std::cerr << err_str << "\n";
        }
        return 1;
    }

    Validator v;
    bool valid = v.validate_world(path, world);
    if (!valid){
        const auto& err_vec = v.get_errors();
        for (size_t i = 0; i < err_vec.size(); i++)
        {
            const std::string& err_str = err_vec.at(i);
            std::cerr << err_str << "\n";
        }
        return 1;
    }

    Aggregation agg;
    EngineOptions opts; opts.filter = filter;
    std::string err_str;
    if (!process_world(world, opts, agg, err_str)) {
        std::cerr << err_str << "\n";
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
