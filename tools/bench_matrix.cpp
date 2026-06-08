#include "bench/bench_args.h"
#include "bench/bench_csv.h"
#include "bench/bench_runner.h"
#include "core/prepared_keys.h"
#include "json_input.h"
#include "key_generation.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using bench::KeyCase;
using bench::make_key_cases;
using bench::make_teddy_configs;
using bench::mode_includes_bench;
using bench::mode_includes_stats;
using bench::Options;
using bench::parse_options;
using bench::print_dry_run;
using bench::run_bench_case;
using bench::run_stats_case;
using bench::TeddyConfigCase;
using bench::write_bench_header;
using bench::write_stats_header;

namespace {

PreparedKeys generate_keys_for_case(const json_input::LoadedJson& loaded_json,
                                    const KeyCase& key_case) {
    std::vector<std::string> keys =
        keygen::generate_keys(loaded_json.key_catalog(), key_case.key_type,
                              key_case.num_keys, key_case.seed);

    PreparedKeys prepared = prepare_keys(std::move(keys));
    if (prepared.keys.empty()) {
        std::cerr << "Generated zero keys for " << key_case.json_path << " ("
                  << keygen::key_type_name(key_case.key_type) << ")\n";
        std::exit(EXIT_FAILURE);
    }

    return prepared;
}

void run_matrix(const Options& options,
                const std::vector<KeyCase>& key_cases,
                const std::vector<TeddyConfigCase>& teddy_configs) {
    std::filesystem::create_directories(options.out_dir);

    std::ofstream bench_output;
    std::ofstream stats_output;
    if (mode_includes_bench(options.mode)) {
        bench_output.open(options.out_dir / "bench.csv");
        if (!bench_output) {
            std::cerr << "Failed to open bench.csv for writing\n";
            std::exit(EXIT_FAILURE);
        }
        write_bench_header(bench_output);
    }
    if (mode_includes_stats(options.mode)) {
        stats_output.open(options.out_dir / "stats.csv");
        if (!stats_output) {
            std::cerr << "Failed to open stats.csv for writing\n";
            std::exit(EXIT_FAILURE);
        }
        write_stats_header(stats_output);
    }

    std::unordered_map<std::string, json_input::LoadedJson> loaded_jsons;
    for (const std::string& json_path : options.json_paths) {
        loaded_jsons.emplace(json_path,
                             json_input::load_json_or_exit(json_path));
    }

    for (const KeyCase& key_case : key_cases) {
        const json_input::LoadedJson& loaded_json =
            loaded_jsons.at(key_case.json_path);
        PreparedKeys keys = generate_keys_for_case(loaded_json, key_case);

        if (mode_includes_bench(options.mode)) {
            for (const findkey_algo algo : options.algos) {
                if (algo == SCALAR) {
                    run_bench_case(bench_output, loaded_json.data(), key_case,
                                   keys, algo, std::nullopt, options.repeats,
                                   options.warmup);
                    continue;
                }

                for (const TeddyConfigCase& teddy_config : teddy_configs) {
                    run_bench_case(bench_output, loaded_json.data(), key_case,
                                   keys, algo, teddy_config, options.repeats,
                                   options.warmup);
                }
            }
        }

        if (mode_includes_stats(options.mode)) {
            for (const TeddyConfigCase& teddy_config : teddy_configs) {
                run_stats_case(stats_output, loaded_json.data(), key_case, keys,
                               teddy_config, options.repeats, options.warmup);
            }
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    const Options options = parse_options(argc, argv);
    const std::vector<KeyCase> key_cases = make_key_cases(options);
    const std::vector<TeddyConfigCase> teddy_configs =
        make_teddy_configs(options);

    if (options.dry_run) {
        print_dry_run(options, key_cases, teddy_configs);
        return EXIT_SUCCESS;
    }

    run_matrix(options, key_cases, teddy_configs);

    if (mode_includes_bench(options.mode)) {
        std::cout << "Wrote " << (options.out_dir / "bench.csv") << '\n';
    }
    if (mode_includes_stats(options.mode)) {
        std::cout << "Wrote " << (options.out_dir / "stats.csv") << '\n';
    }

    return EXIT_SUCCESS;
}
