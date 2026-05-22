#include "cli/args.h"

#include <getopt.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

[[noreturn]] void print_usage_and_exit(const char* prog_name) {
    static constexpr const char* usage_message =
        "Usage:\n"
        "  %s --keys <keys_file> --data <json_file> [options]\n"
        "\n"
        "Required:\n"
        "  --keys <keys_file>         Newline-delimited list of keys to "
        "search\n"
        "  --data <json_file>         JSON input file to scan\n"
        "\n"
        "General options:\n"
        "  --algo <name>              Matching algorithm\n"
        "                             Values: scalar, teddy, teddy_baseline\n"
        "                             Default: scalar\n"
        "  --print-positions          Print the position and key for each "
        "match\n"
        "  --collect-stats            Print Teddy baseline false-positive "
        "stats\n"
        "\n"
        "Teddy options:\n"
        "  --teddy-grouping-strategy <name>\n"
        "                             Values: paper_greedy, improved_greedy, "
        "hash\n"
        "                             Alias: greedy = paper_greedy\n"
        "                             Default: paper_greedy\n"
        "  --teddy-hash-algo <name>   Used only with grouping strategy 'hash'\n"
        "                             Values: std, adler32, crc32, xxhash, "
        "fnv1a\n"
        "                             Default: std\n"
        "  --teddy-suffix-mode <name>\n"
        "                             Values: raw, quote-suffix\n"
        "                             Default: raw\n"
        "\n"
        "Notes:\n"
        "  - --collect-stats always uses the Teddy baseline matcher\n"
        "  - Teddy options are ignored when --algo scalar is selected\n";

    std::fprintf(stderr, usage_message, prog_name);
    std::exit(EXIT_FAILURE);
}

template <typename Enum>
Enum invalid_enum_value() {
    return static_cast<Enum>(-1);
}

findkey_algo parse_algo(const char* algo) {
    if (std::strcmp(algo, "scalar") == 0) {
        return SCALAR;
    }
    if (std::strcmp(algo, "teddy") == 0) {
        return TEDDY;
    }
    if (std::strcmp(algo, "teddy_baseline") == 0) {
        return TEDDY_BASELINE;
    }
    return invalid_enum_value<findkey_algo>();
}

findkey_teddy_compile_grouping_strategy parse_grouping_strategy(
    const char* strategy) {
    if (std::strcmp(strategy, "paper_greedy") == 0 ||
        std::strcmp(strategy, "greedy") == 0) {
        return TEDDY_COMPILE_PAPER_GREEDY;
    }
    if (std::strcmp(strategy, "improved_greedy") == 0) {
        return TEDDY_COMPILE_PAPER_IMPROVED_GREEDY;
    }
    if (std::strcmp(strategy, "hash") == 0) {
        return TEDDY_COMPILE_HASH;
    }
    return invalid_enum_value<findkey_teddy_compile_grouping_strategy>();
}

findkey_teddy_suffix_mode parse_suffix_mode(const char* mode) {
    if (std::strcmp(mode, "raw") == 0) {
        return TEDDY_SUFFIX_RAW;
    }
    if (std::strcmp(mode, "quote-suffix") == 0) {
        return TEDDY_SUFFIX_QUOTED;
    }
    return invalid_enum_value<findkey_teddy_suffix_mode>();
}

findkey_teddy_compile_hash_algorithm parse_hash_algorithm(
    const char* algorithm) {
    if (std::strcmp(algorithm, "std") == 0) {
        return TEDDY_HASH_STD;
    }
    if (std::strcmp(algorithm, "adler32") == 0) {
        return TEDDY_HASH_ADLER32;
    }
    if (std::strcmp(algorithm, "crc32") == 0) {
        return TEDDY_HASH_CRC32;
    }
    if (std::strcmp(algorithm, "xxhash") == 0) {
        return TEDDY_HASH_XXHASH;
    }
    if (std::strcmp(algorithm, "fnv1a") == 0) {
        return TEDDY_HASH_FNV1A;
    }
    return invalid_enum_value<findkey_teddy_compile_hash_algorithm>();
}

template <typename Enum>
void validate_enum_or_exit(Enum value,
                           Enum invalid_value,
                           const char* error_message,
                           const char* prog_name) {
    if (value == invalid_value) {
        std::fprintf(stderr, "%s\n", error_message);
        print_usage_and_exit(prog_name);
    }
}

}  // namespace

ParsedCliArgs parse_cli_args_or_exit(int argc, char** argv) {
    static constexpr option long_options[] = {
        {"algo", required_argument, nullptr, 'a'},
        {"teddy-grouping-strategy", required_argument, nullptr, 'g'},
        {"teddy-hash-algo", required_argument, nullptr, 'h'},
        {"teddy-suffix-mode", required_argument, nullptr, 's'},
        {"keys", required_argument, nullptr, 'k'},
        {"data", required_argument, nullptr, 'd'},
        {"collect-stats", no_argument, nullptr, 'c'},
        {"print-positions", no_argument, nullptr, 'p'},
        {nullptr, 0, nullptr, 0},
    };

    ParsedCliArgs args;

    opterr = 0;
    while (true) {
        const int option_value =
            getopt_long(argc, argv, "", long_options, nullptr);
        if (option_value == -1) {
            break;
        }

        switch (option_value) {
            case 'a':
                args.algo = parse_algo(optarg);
                break;
            case 'g':
                args.teddy_config.grouping_strategy =
                    parse_grouping_strategy(optarg);
                break;
            case 'h':
                args.teddy_config.hash_algorithm = parse_hash_algorithm(optarg);
                break;
            case 's':
                args.teddy_config.suffix_mode = parse_suffix_mode(optarg);
                break;
            case 'k':
                args.keys_path = optarg;
                break;
            case 'd':
                args.data_path = optarg;
                break;
            case 'c':
                args.collect_stats = true;
                break;
            case 'p':
                args.print_positions = true;
                break;
            default:
                print_usage_and_exit(argv[0]);
        }
    }

    if (optind != argc) {
        print_usage_and_exit(argv[0]);
    }

    validate_enum_or_exit(args.algo, invalid_enum_value<findkey_algo>(),
                          "Unknown algorithm specified", argv[0]);
    validate_enum_or_exit(
        args.teddy_config.grouping_strategy,
        invalid_enum_value<findkey_teddy_compile_grouping_strategy>(),
        "Unknown teddy grouping strategy specified", argv[0]);
    validate_enum_or_exit(args.teddy_config.suffix_mode,
                          invalid_enum_value<findkey_teddy_suffix_mode>(),
                          "Unknown teddy suffix mode specified", argv[0]);
    validate_enum_or_exit(
        args.teddy_config.hash_algorithm,
        invalid_enum_value<findkey_teddy_compile_hash_algorithm>(),
        "Unknown teddy hash algorithm specified", argv[0]);

    if (!args.keys_path || !args.data_path) {
        print_usage_and_exit(argv[0]);
    }

    return args;
}
