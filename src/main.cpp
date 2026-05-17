#include "findkey.h"
#include "io/mmap_file.h"

#include <getopt.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <chrono>

static void print_usage_and_exit(const char* prog_name) {
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
        "                             Values: greedy, hash\n"
        "                             Default: greedy\n"
        "  --teddy-hash-algo <name>   Used only with grouping strategy 'hash'\n"
        "                             Values: std, adler32, crc32fast, xxhash, "
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

static std::vector<std::string> read_keys_from_file(const char* keys_file) {
    std::ifstream infile(keys_file);
    if (!infile) {
        std::fprintf(stderr, "Failed to open keys file: %s\n", keys_file);
        std::exit(EXIT_FAILURE);
    }

    std::vector<std::string> keys;
    std::string line;
    while (std::getline(infile, line)) {
        if (!line.empty()) {
            if (line.back() == '\r') {
                line.pop_back();  // windows return
            }
            keys.push_back(line);
        }
    }

    if (keys.empty()) {
        std::fprintf(stderr, "No keys found in keys file: %s\n", keys_file);
        std::exit(EXIT_FAILURE);
    }

    return keys;
}

static findkey_algo parse_algo(const char* algo) {
    if (std::strcmp(algo, "scalar") == 0) {
        return SCALAR;
    }

    if (std::strcmp(algo, "teddy") == 0) {
        return TEDDY;
    }

    if (std::strcmp(algo, "teddy_baseline") == 0) {
        return TEDDY_BASELINE;
    }

    return static_cast<findkey_algo>(-1);
}

static findkey_teddy_compile_grouping_strategy parse_grouping_strategy(
    const char* strategy) {
    if (std::strcmp(strategy, "greedy") == 0) {
        return TEDDY_COMPILE_GREEDY;
    }

    if (std::strcmp(strategy, "hash") == 0) {
        return TEDDY_COMPILE_HASH;
    }

    return static_cast<findkey_teddy_compile_grouping_strategy>(-1);
}

static findkey_teddy_suffix_mode parse_suffix_mode(const char* mode) {
    if (std::strcmp(mode, "raw") == 0) {
        return TEDDY_SUFFIX_RAW;
    }

    if (std::strcmp(mode, "quote-suffix") == 0) {
        return TEDDY_SUFFIX_QUOTED;
    }

    return static_cast<findkey_teddy_suffix_mode>(-1);
}

static findkey_teddy_compile_hash_algorithm parse_hash_algorithm(
    const char* algorithm) {
    if (std::strcmp(algorithm, "std") == 0) {
        return TEDDY_HASH_STD;
    }

    if (std::strcmp(algorithm, "adler32") == 0) {
        return TEDDY_HASH_ADLER32;
    }

    if (std::strcmp(algorithm, "crc32fast") == 0) {
        return TEDDY_HASH_CRC32FAST;
    }

    if (std::strcmp(algorithm, "xxhash") == 0) {
        return TEDDY_HASH_XXHASH;
    }

    if (std::strcmp(algorithm, "fnv1a") == 0) {
        return TEDDY_HASH_FNV1A;
    }

    return static_cast<findkey_teddy_compile_hash_algorithm>(-1);
}

int main(int argc, char** argv) {
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

    enum findkey_algo algo = SCALAR;
    findkey_teddy_config teddy_config = {};
    const char* keys_path = nullptr;
    const char* data_path = nullptr;
    bool collect_stats = false;
    bool print_positions = false;

    opterr = 0;
    while (true) {
        const int option_value =
            getopt_long(argc, argv, "", long_options, nullptr);
        if (option_value == -1) {
            break;
        }

        switch (option_value) {
            case 'a':
                algo = parse_algo(optarg);
                break;
            case 'g':
                teddy_config.grouping_strategy =
                    parse_grouping_strategy(optarg);
                break;
            case 'h':
                teddy_config.hash_algorithm = parse_hash_algorithm(optarg);
                break;
            case 's':
                teddy_config.suffix_mode = parse_suffix_mode(optarg);
                break;
            case 'k':
                keys_path = optarg;
                break;
            case 'd':
                data_path = optarg;
                break;
            case 'c':
                collect_stats = true;
                break;
            case 'p':
                print_positions = true;
                break;
            default:
                print_usage_and_exit(argv[0]);
        }
    }

    if (optind != argc) {
        print_usage_and_exit(argv[0]);
    }

    if (algo == static_cast<findkey_algo>(-1)) {
        std::fprintf(stderr, "Unknown algorithm specified\n");
        print_usage_and_exit(argv[0]);
    }

    if (teddy_config.grouping_strategy ==
        static_cast<findkey_teddy_compile_grouping_strategy>(-1)) {
        std::fprintf(stderr, "Unknown teddy grouping strategy specified\n");
        print_usage_and_exit(argv[0]);
    }

    if (teddy_config.suffix_mode ==
        static_cast<findkey_teddy_suffix_mode>(-1)) {
        std::fprintf(stderr, "Unknown teddy suffix mode specified\n");
        print_usage_and_exit(argv[0]);
    }

    if (teddy_config.hash_algorithm ==
        static_cast<findkey_teddy_compile_hash_algorithm>(-1)) {
        std::fprintf(stderr, "Unknown teddy hash algorithm specified\n");
        print_usage_and_exit(argv[0]);
    }

    if (!keys_path || !data_path) {
        print_usage_and_exit(argv[0]);
    }

    std::vector<std::string> keys = read_keys_from_file(keys_path);
    std::vector<const uint8_t*> key_ptrs;
    key_ptrs.reserve(keys.size());
    std::vector<size_t> key_lens;
    key_lens.reserve(keys.size());
    for (const auto& key : keys) {
        key_ptrs.push_back(reinterpret_cast<const uint8_t*>(key.data()));
        key_lens.push_back(key.size());
    }

    MMapFile mmap_file(data_path);

    constexpr size_t POSITIONS_CAPACITY = 1024 * 1024;
    std::vector<findkey_result> positions(POSITIONS_CAPACITY);
    int status = 0;
    findkey_teddy_stats teddy_stats = {};

    auto start_time = std::chrono::steady_clock::now();

    size_t num_found =
        collect_stats
            ? findkey_with_stats(
                  reinterpret_cast<const uint8_t*>(mmap_file.data()),
                  mmap_file.size(), key_ptrs.data(), key_lens.data(),
                  key_ptrs.size(), &teddy_config, &teddy_stats, &status)
            : findkey(reinterpret_cast<const uint8_t*>(mmap_file.data()),
                      mmap_file.size(), key_ptrs.data(), key_lens.data(),
                      key_ptrs.size(), algo, &teddy_config, positions.data(),
                      positions.size(), &status);

    auto end_time = std::chrono::steady_clock::now();

    const auto duration_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time -
                                                             start_time)
            .count();
    const double duration_s = duration_ns / 1e9;

    const double bytes = mmap_file.size();
    const double mbps =
        duration_s > 0 ? (bytes / (1024.0 * 1024.0)) / duration_s : 0.0;

    switch (status) {
        case FINDKEY_ERR_BAD_ARGS:
            std::fprintf(stderr, "Bad arguments\n");
            return EXIT_FAILURE;
        case FINDKEY_TEDDY_NOT_SUPPORTED:
            std::fprintf(stderr, "Teddy not supported by this compiler\n");
            return EXIT_FAILURE;
        case FINDKEY_ERR_UNKNOWN_ALGO:
            std::fprintf(stderr, "Unknown algorithm specified\n");
            return EXIT_FAILURE;
        default:
            break;
    }

    std::printf("Total key-value pairs found: %zu\n", num_found);

    if (print_positions) {
        for (size_t i = 0; i < num_found && i < positions.size(); ++i) {
            std::printf("\tPosition: %zu\n", positions[i].position);
            std::printf("\tKey: \"%s\"\n", keys[positions[i].key_id].c_str());
        }
        if (num_found > positions.size()) {
            std::printf("  ... and %zu more\n", num_found - positions.size());
        }
    }

    if (collect_stats) {
        std::printf("Teddy Stats:\n");
        std::printf("\tPrefilter hit lanes: %lu\n",
                    teddy_stats.prefilter_hit_lanes);
        std::printf("\tPrefilter hit groups: %lu\n",
                    teddy_stats.prefilter_hit_groups);
        std::printf("\tFP type 1 lanes: %lu\n", teddy_stats.fp_type1_lanes);
        std::printf("\tFP type 1 groups: %lu\n", teddy_stats.fp_type1_groups);
        std::printf("\tFP type 2 lanes: %lu\n", teddy_stats.fp_type2_lanes);
        std::printf("\tReject bad end quote: %lu\n",
                    teddy_stats.reject_bad_end_quote);
        std::printf("\tReject invalid quote: %lu\n",
                    teddy_stats.reject_invalid_quote);
        std::printf("\tReject missing colon: %lu\n",
                    teddy_stats.reject_missing_colon);
        std::printf("\tReject missing open quote: %lu\n",
                    teddy_stats.reject_missing_open_quote);
        std::printf("\tReject key not found: %lu\n",
                    teddy_stats.reject_key_not_found);
        std::printf("\tExact matches: %lu\n", teddy_stats.exact_matches);
    }

    std::printf("Time taken: %.2f ns\n", static_cast<double>(duration_ns));
    std::printf("Data size: %.2f MiB\n", bytes / (1024.0 * 1024.0));
    std::printf("Throughput: %.2f MiB/s\n", mbps);

    return 0;
}
