#include "cli/args.h"
#include "core/findkey_options.h"

#include <getopt.h>

#include <cstdio>
#include <cstdlib>

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
        "hash_std, hash_adler32, hash_crc32, hash_xxhash, hash_fnv1a, "
        "sorted_suffix_round_robin, sorted_suffix_partition\n"
        "                             Alias: greedy = paper_greedy\n"
        "                             Default: paper_greedy\n"
        "  --teddy-suffix-mode <name>\n"
        "                             Values: raw, quote-suffix\n"
        "                             Default: raw\n"
        "  --sigma <n>                Suffix length for teddy keys grouping\n"
        "                             Range: 1..4\n"
        "                             Default: 3\n"
        "\n"
        "Notes:\n"
        "  - --collect-stats always uses the Teddy baseline matcher\n"
        "  - Teddy options are ignored when --algo scalar is selected\n";

    std::fprintf(stderr, usage_message, prog_name);
    std::exit(EXIT_FAILURE);
}

}  // namespace

ParsedCliArgs parse_cli_args_or_exit(int argc, char** argv) {
    static constexpr option long_options[] = {
        {"algo", required_argument, nullptr, 'a'},
        {"teddy-grouping-strategy", required_argument, nullptr, 'g'},
        {"teddy-suffix-mode", required_argument, nullptr, 's'},
        {"sigma", required_argument, nullptr, 'm'},
        {"keys", required_argument, nullptr, 'k'},
        {"data", required_argument, nullptr, 'd'},
        {"collect-stats", no_argument, nullptr, 'c'},
        {"print-positions", no_argument, nullptr, 'p'},
        {nullptr, 0, nullptr, 0},
    };

    ParsedCliArgs args;

    opterr = 0;
    optind = 1;
    while (true) {
        const int option_value =
            getopt_long(argc, argv, "", long_options, nullptr);
        if (option_value == -1) {
            break;
        }

        switch (option_value) {
            case 'a': {
                const auto parsed = findkey_options::parse_algo(optarg);
                if (!parsed) {
                    std::fprintf(stderr, "Unknown algorithm specified\n");
                    print_usage_and_exit(argv[0]);
                }
                args.algo = *parsed;
                break;
            }
            case 'g': {
                const auto parsed =
                    findkey_options::parse_grouping_strategy(optarg);
                if (!parsed) {
                    std::fprintf(stderr,
                                 "Unknown teddy grouping strategy specified\n");
                    print_usage_and_exit(argv[0]);
                }
                args.teddy_config.grouping_strategy = *parsed;
                break;
            }
            case 's': {
                const auto parsed = findkey_options::parse_suffix_mode(optarg);
                if (!parsed) {
                    std::fprintf(stderr,
                                 "Unknown teddy suffix mode specified\n");
                    print_usage_and_exit(argv[0]);
                }
                args.teddy_config.suffix_mode = *parsed;
                break;
            }
            case 'm': {
                const auto parsed = findkey_options::parse_sigma(optarg);
                if (!parsed) {
                    std::fprintf(stderr, "Invalid sigma specified\n");
                    print_usage_and_exit(argv[0]);
                }
                args.teddy_config.sigma = *parsed;
                break;
            }
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

    if (!args.keys_path || !args.data_path) {
        print_usage_and_exit(argv[0]);
    }

    return args;
}
