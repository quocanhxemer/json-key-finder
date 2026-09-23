#include "cli/args.h"
#include "cli/reporting.h"
#include "core/prepared_keys.h"
#include "findkey.h"
#include "io/mmap_file.h"
#include "teddy/compile.h"
#include "teddy/verification/dispatch.h"
#include "teddy/verification/metadata.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static std::vector<std::string> read_keys_from_file(const char* keys_file) {
    std::ifstream infile(keys_file);
    if (!infile) {
        std::cerr << "Failed to open keys file: " << keys_file << '\n';
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
        std::cerr << "No keys found in keys file: " << keys_file << '\n';
        std::exit(EXIT_FAILURE);
    }

    return keys;
}

int main(int argc, char** argv) {
    const ParsedCliArgs args = parse_cli_args_or_exit(argc, argv);

    PreparedKeys keys = prepare_keys(read_keys_from_file(args.keys_path));
    if (keys.keys.empty()) {
        std::cerr << "No keys found in keys file: " << args.keys_path << '\n';
        std::exit(EXIT_FAILURE);
    }

    MMapFile mmap_file(args.data_path);

    constexpr size_t POSITIONS_CAPACITY = 1024 * 1024;
    std::vector<findkey_result> positions(POSITIONS_CAPACITY);
    int status = 0;
    findkey_teddy_stats teddy_stats = {};
    findkey_timing timing = {};
    teddy::CompilationMetadata teddy_compilation_metadata = {};
    teddy::VerifierCompilationMetadata verifier_compilation_metadata = {};

    if (args.collect_stats) {
        const teddy::CompilationData teddy_data =
            teddy::compile(keys.views, args.teddy_config);
        teddy_compilation_metadata =
            teddy::get_compilation_metadata(teddy_data);
        verifier_compilation_metadata = teddy::dispatch_verifier(
            args.teddy_config.verification_strategy,
            [&]<teddy::Verifier VerifierModel>() {
                const teddy::VerificationBuildContext context{keys.views,
                                                              teddy_data};
                const VerifierModel verifier(context);
                return teddy::get_verifier_compilation_metadata(verifier);
            });
    }

    size_t num_found =
        args.collect_stats
            ? findkey_with_stats(
                  reinterpret_cast<const uint8_t*>(mmap_file.data()),
                  mmap_file.size(), keys.ptrs.data(), keys.lens.data(),
                  keys.ptrs.size(), &args.teddy_config, &teddy_stats, &status)
            : findkey(reinterpret_cast<const uint8_t*>(mmap_file.data()),
                      mmap_file.size(), keys.ptrs.data(), keys.lens.data(),
                      keys.ptrs.size(), args.algo, &args.teddy_config,
                      positions.data(), positions.size(), &status, &timing);

    switch (status) {
        case FINDKEY_ERR_BAD_ARGS:
            std::cerr << "Bad arguments\n";
            return EXIT_FAILURE;
        case FINDKEY_TEDDY_NOT_SUPPORTED:
            std::cerr << "Teddy not supported by this compiler\n";
            return EXIT_FAILURE;
        case FINDKEY_ERR_UNKNOWN_ALGO:
            std::cerr << "Unknown algorithm specified\n";
            return EXIT_FAILURE;
        default:
            break;
    }

    std::cout << "Total key-value pairs found: " << num_found << '\n';

    if (args.print_positions) {
        for (size_t i = 0; i < num_found && i < positions.size(); ++i) {
            std::cout << "\tPosition: " << positions[i].position << '\n';
            std::cout << "\tKey: \"" << keys.keys[positions[i].key_id]
                      << "\"\n";
        }
        if (num_found > positions.size()) {
            std::cout << "  ... and " << num_found - positions.size()
                      << " more\n";
        }
    }

    if (args.collect_stats) {
        print_compilation_stats(teddy_compilation_metadata,
                                verifier_compilation_metadata);
        print_teddy_runtime_stats(teddy_stats, mmap_file.size());
    } else {
        const uint64_t total_ns =
            timing.compile_ns + timing.verifier_build_ns + timing.match_ns;
        const double total_duration_s = total_ns / 1e9;
        const double match_duration_s = timing.match_ns / 1e9;

        const double bytes = mmap_file.size();
        const double mbps = match_duration_s > 0
                                ? (bytes / (1024.0 * 1024.0)) / match_duration_s
                                : 0.0;
        const double end_to_end_mbps =
            total_duration_s > 0
                ? (bytes / (1024.0 * 1024.0)) / total_duration_s
                : 0.0;

        std::cout << "Compile time: " << timing.compile_ns << " ns\n";
        std::cout << "Verifier build time: " << timing.verifier_build_ns
                  << " ns\n";
        std::cout << "Match time: " << timing.match_ns << " ns\n";
        std::cout << "Time taken: " << total_ns << " ns\n";
        std::cout << "Data size: " << bytes / (1024.0 * 1024.0) << " MiB\n";
        std::cout << "Throughput: " << mbps << " MiB/s\n";
        std::cout << "End-to-end throughput: " << end_to_end_mbps << " MiB/s\n";
    }

    return 0;
}
