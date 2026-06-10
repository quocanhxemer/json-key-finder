#include "bench/bench_args.h"

#include "core/findkey_options.h"

#include <getopt.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

namespace bench {
namespace {

std::optional<RunMode> parse_mode(std::string_view raw) {
    if (raw == "bench") {
        return RunMode::Bench;
    }
    if (raw == "stats") {
        return RunMode::Stats;
    }
    if (raw == "both") {
        return RunMode::Both;
    }
    return std::nullopt;
}

std::optional<size_t> parse_size(std::string_view raw) {
    if (raw.empty()) {
        return std::nullopt;
    }

    char* end = nullptr;
    const std::string text(raw);
    const unsigned long long value = std::strtoull(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0') {
        return std::nullopt;
    }
    return static_cast<size_t>(value);
}

[[noreturn]] void print_usage_and_exit(const char* program_name) {
    std::cerr
        << "Usage:\n"
        << "  " << program_name
        << " --json <file> [--json <file> ...] [options]\n\n"
        << "General options:\n"
        << "  --mode <bench|stats|both>        Default: both\n"
        << "  --out-dir <dir>                  Default: bench_out_cpp\n"
        << "  --repeats <n>                    Default: 5\n"
        << "  --warmup <n>                     Default: 1\n"
        << "  --dry-run                        Print matrix size and exit\n\n"
        << "Key generation options:\n"
        << "  --key-type <type>                Repeatable. Defaults: highest, "
           "mixed, no_match\n"
        << "                                  Values: highest, lowest, "
           "no_match, all_match, mixed, all\n"
        << "  --num-keys <n>                   Repeatable. Defaults: 16, 32, "
           "60\n"
        << "  --seed <n>                       Repeatable. Default: 1\n\n"
        << "Benchmark options:\n"
        << "  --algo <name>                    Repeatable. Defaults: scalar, "
           "teddy, teddy_baseline\n"
        << "  --grouping <name>                Repeatable. Defaults: "
           "paper_greedy, improved_greedy, hash_std, hash_xxhash, "
           "hash_crc32, hash_fnv1a\n"
        << "  --suffix-mode <name>             Repeatable. Defaults: raw, "
           "quote-suffix\n"
        << "  --sigma <n>                      Repeatable. Defaults: 1, 2, 3, "
           "4\n";
    std::exit(EXIT_FAILURE);
}

bool has_non_scalar_algo(const Options& options) {
    return std::any_of(options.algos.begin(), options.algos.end(),
                       [](findkey_algo algo) { return algo != SCALAR; });
}

}  // namespace

Options parse_options(int argc, char** argv) {
    static constexpr option long_options[] = {
        {"json", required_argument, nullptr, 'j'},
        {"mode", required_argument, nullptr, 'm'},
        {"out-dir", required_argument, nullptr, 'o'},
        {"key-type", required_argument, nullptr, 'k'},
        {"num-keys", required_argument, nullptr, 'n'},
        {"seed", required_argument, nullptr, 's'},
        {"algo", required_argument, nullptr, 'a'},
        {"grouping", required_argument, nullptr, 'g'},
        {"suffix-mode", required_argument, nullptr, 'f'},
        {"sigma", required_argument, nullptr, 'i'},
        {"repeats", required_argument, nullptr, 'r'},
        {"warmup", required_argument, nullptr, 'w'},
        {"dry-run", no_argument, nullptr, 'd'},
        {nullptr, 0, nullptr, 0},
    };

    Options options;

    opterr = 0;
    optind = 1;
    while (true) {
        const int option_value =
            getopt_long(argc, argv, "", long_options, nullptr);
        if (option_value == -1) {
            break;
        }

        switch (option_value) {
            case 'j':
                options.json_paths.emplace_back(optarg);
                break;
            case 'm': {
                const auto mode = parse_mode(optarg);
                if (!mode) {
                    std::cerr << "Invalid --mode\n";
                    print_usage_and_exit(argv[0]);
                }
                options.mode = *mode;
                break;
            }
            case 'o':
                options.out_dir = std::filesystem::path(optarg);
                break;
            case 'k': {
                const auto key_type = keygen::parse_key_type(optarg);
                if (!key_type) {
                    std::cerr << "Invalid --key-type\n";
                    print_usage_and_exit(argv[0]);
                }
                options.key_types.push_back(*key_type);
                break;
            }
            case 'n': {
                const auto value = parse_size(optarg);
                if (!value) {
                    std::cerr << "Invalid --num-keys\n";
                    print_usage_and_exit(argv[0]);
                }
                options.num_keys.push_back(*value);
                break;
            }
            case 's': {
                const auto value = parse_size(optarg);
                if (!value) {
                    std::cerr << "Invalid --seed\n";
                    print_usage_and_exit(argv[0]);
                }
                options.seeds.push_back(static_cast<uint32_t>(*value));
                break;
            }
            case 'a': {
                const auto algo = findkey_options::parse_algo(optarg);
                if (!algo) {
                    std::cerr << "Invalid --algo\n";
                    print_usage_and_exit(argv[0]);
                }
                options.algos.push_back(*algo);
                break;
            }
            case 'g': {
                const auto grouping =
                    findkey_options::parse_grouping_strategy(optarg);
                if (!grouping) {
                    std::cerr << "Invalid --grouping\n";
                    print_usage_and_exit(argv[0]);
                }
                options.grouping_strategies.push_back(*grouping);
                break;
            }
            case 'f': {
                const auto suffix_mode =
                    findkey_options::parse_suffix_mode(optarg);
                if (!suffix_mode) {
                    std::cerr << "Invalid --suffix-mode\n";
                    print_usage_and_exit(argv[0]);
                }
                options.suffix_modes.push_back(*suffix_mode);
                break;
            }
            case 'i': {
                const auto sigma = findkey_options::parse_sigma(optarg);
                if (!sigma) {
                    std::cerr << "Invalid --sigma\n";
                    print_usage_and_exit(argv[0]);
                }
                options.sigmas.push_back(*sigma);
                break;
            }
            case 'r': {
                const auto value = parse_size(optarg);
                if (!value) {
                    std::cerr << "Invalid --repeats\n";
                    print_usage_and_exit(argv[0]);
                }
                options.repeats = *value;
                break;
            }
            case 'w': {
                const auto value = parse_size(optarg);
                if (!value) {
                    std::cerr << "Invalid --warmup\n";
                    print_usage_and_exit(argv[0]);
                }
                options.warmup = *value;
                break;
            }
            case 'd':
                options.dry_run = true;
                break;
            default:
                print_usage_and_exit(argv[0]);
        }
    }

    if (optind != argc) {
        print_usage_and_exit(argv[0]);
    }

    if (options.json_paths.empty()) {
        std::cerr << "At least one --json input is required\n";
        print_usage_and_exit(argv[0]);
    }
    if (options.key_types.empty()) {
        options.key_types = {keygen::KeyType::Highest, keygen::KeyType::Mixed,
                             keygen::KeyType::NoMatch};
    }
    if (options.num_keys.empty()) {
        options.num_keys = {16, 32, 60};
    }
    if (options.seeds.empty()) {
        options.seeds = {1};
    }
    if (options.algos.empty()) {
        options.algos = {SCALAR, TEDDY, TEDDY_BASELINE};
    }
    if (options.grouping_strategies.empty()) {
        options.grouping_strategies = {
            TEDDY_COMPILE_PAPER_GREEDY, TEDDY_COMPILE_PAPER_IMPROVED_GREEDY,
            TEDDY_COMPILE_HASH_STD,     TEDDY_COMPILE_HASH_XXHASH,
            TEDDY_COMPILE_HASH_CRC32,   TEDDY_COMPILE_HASH_FNV1A};
    }
    if (options.suffix_modes.empty()) {
        options.suffix_modes = {TEDDY_SUFFIX_RAW, TEDDY_SUFFIX_QUOTED};
    }
    if (options.sigmas.empty()) {
        options.sigmas = {1, 2, 3, 4};
    }

    return options;
}

bool mode_includes_bench(RunMode mode) {
    return mode == RunMode::Bench || mode == RunMode::Both;
}

bool mode_includes_stats(RunMode mode) {
    return mode == RunMode::Stats || mode == RunMode::Both;
}

std::vector<TeddyConfigCase> make_teddy_configs(const Options& options) {
    bool need_configs =
        mode_includes_stats(options.mode) ||
        (mode_includes_bench(options.mode) && has_non_scalar_algo(options));

    if (!need_configs) {
        return {};
    }

    std::vector<TeddyConfigCase> configs;

    for (const auto grouping_strategy : options.grouping_strategies) {
        for (const auto suffix_mode : options.suffix_modes) {
            for (const int sigma : options.sigmas) {
                configs.push_back({{grouping_strategy, suffix_mode, sigma}});
            }
        }
    }

    return configs;
}

std::vector<KeyCase> make_key_cases(const Options& options) {
    std::vector<KeyCase> cases;

    for (const std::string& json_path : options.json_paths) {
        for (const keygen::KeyType key_type : options.key_types) {
            // num_keys & seeds only relevant for KeyType != All
            for (const size_t num_keys : options.num_keys) {
                for (const uint32_t seed : options.seeds) {
                    cases.push_back({json_path, key_type, num_keys, seed});
                    if (key_type == keygen::KeyType::All) {
                        break;
                    }
                }
                if (key_type == keygen::KeyType::All) {
                    break;
                }
            }
        }
    }

    return cases;
}

void print_dry_run(const Options& options,
                   const std::vector<KeyCase>& key_cases,
                   const std::vector<TeddyConfigCase>& teddy_configs) {
    const size_t total_iterations = options.repeats + options.warmup;
    size_t bench_runs = 0;
    size_t stats_runs = 0;

    if (mode_includes_bench(options.mode)) {
        for (const findkey_algo algo : options.algos) {
            bench_runs += key_cases.size() *
                          (algo == SCALAR ? 1 : teddy_configs.size()) *
                          total_iterations;
        }
    }

    if (mode_includes_stats(options.mode)) {
        stats_runs = key_cases.size() * teddy_configs.size() * total_iterations;
    }

    std::cout << "Key cases: " << key_cases.size() << '\n';
    std::cout << "Teddy configs: " << teddy_configs.size() << '\n';
    std::cout << "Total runs: " << (bench_runs + stats_runs)
              << " (bench=" << bench_runs << ", stats=" << stats_runs << ")\n";
    std::cout << "Warmup per case: " << options.warmup
              << ", repeats per case: " << options.repeats << '\n';
}

}  // namespace bench
