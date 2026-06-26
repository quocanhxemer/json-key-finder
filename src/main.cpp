#include "cli/args.h"
#include "cli/reporting.h"
#include "core/prepared_keys.h"
#include "findkey.h"
#include "io/mmap_file.h"
#include "teddy/teddy_compile.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

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

int main(int argc, char** argv) {
    const ParsedCliArgs args = parse_cli_args_or_exit(argc, argv);

    PreparedKeys keys = prepare_keys(read_keys_from_file(args.keys_path));
    if (keys.keys.empty()) {
        std::fprintf(stderr, "No keys found in keys file: %s\n",
                     args.keys_path);
        std::exit(EXIT_FAILURE);
    }

    MMapFile mmap_file(args.data_path);

    constexpr size_t POSITIONS_CAPACITY = 1024 * 1024;
    std::vector<findkey_result> positions(POSITIONS_CAPACITY);
    int status = 0;
    findkey_teddy_stats teddy_stats = {};
    findkey_timing timing = {};
    TeddyCompilationMetadata teddy_compilation_metadata = {};

    if (args.collect_stats) {
        const TeddyCompilationData teddy_data =
            compile_teddy_data(keys.views, args.teddy_config);
        teddy_compilation_metadata = get_teddy_compilation_metadata(teddy_data);
    }

    size_t num_found =
        args.collect_stats
            ? findkey_with_stats(
                  reinterpret_cast<const uint8_t*>(mmap_file.data()),
                  mmap_file.size(), keys.ptrs.data(), keys.lens.data(),
                  keys.ptrs.size(), &args.teddy_config, &teddy_stats, &status,
                  &timing)
            : findkey(reinterpret_cast<const uint8_t*>(mmap_file.data()),
                      mmap_file.size(), keys.ptrs.data(), keys.lens.data(),
                      keys.ptrs.size(), args.algo, &args.teddy_config,
                      positions.data(), positions.size(), &status, &timing);

    const uint64_t total_ns = timing.compile_ns + timing.match_ns;
    const double total_duration_s = total_ns / 1e9;
    const double match_duration_s = timing.match_ns / 1e9;

    const double bytes = mmap_file.size();
    const double mbps = match_duration_s > 0
                            ? (bytes / (1024.0 * 1024.0)) / match_duration_s
                            : 0.0;
    const double end_to_end_mbps =
        total_duration_s > 0 ? (bytes / (1024.0 * 1024.0)) / total_duration_s
                             : 0.0;

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

    if (args.print_positions) {
        for (size_t i = 0; i < num_found && i < positions.size(); ++i) {
            std::printf("\tPosition: %zu\n", positions[i].position);
            std::printf("\tKey: \"%s\"\n",
                        keys.keys[positions[i].key_id].c_str());
        }
        if (num_found > positions.size()) {
            std::printf("  ... and %zu more\n", num_found - positions.size());
        }
    }

    if (args.collect_stats) {
        print_teddy_compilation_stats(teddy_compilation_metadata);
        print_teddy_runtime_stats(teddy_stats, mmap_file.size());
    }

    std::printf("Compile time: %.2f ns\n",
                static_cast<double>(timing.compile_ns));
    std::printf("Match time: %.2f ns\n", static_cast<double>(timing.match_ns));
    std::printf("Time taken: %.2f ns\n", static_cast<double>(total_ns));
    std::printf("Data size: %.2f MiB\n", bytes / (1024.0 * 1024.0));
    std::printf("Throughput: %.2f MiB/s\n", mbps);
    std::printf("End-to-end throughput: %.2f MiB/s\n", end_to_end_mbps);

    return 0;
}
