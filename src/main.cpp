#include "findkey.h"
#include "mmap_file.h"

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
    std::fprintf(stderr,
                 "Usage: %s --keys <keys_file> --data <json_file> [--algo "
                 "scalar|teddy] [--print-positions]\n",
                 prog_name);
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

int main(int argc, char** argv) {
    const char* algo = "scalar";
    const char* keys_path = nullptr;
    const char* data_path = nullptr;
    bool print_positions = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--algo") == 0 && i + 1 < argc) {
            algo = argv[++i];
        } else if (std::strcmp(argv[i], "--keys") == 0 && i + 1 < argc) {
            keys_path = argv[++i];
        } else if (std::strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
            data_path = argv[++i];
        } else if (std::strcmp(argv[i], "--print-positions") == 0) {
            print_positions = true;
        } else {
            print_usage_and_exit(argv[0]);
        }
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

    auto start_time = std::chrono::steady_clock::now();

    size_t num_found = 0;
    if (std::strcmp(algo, "scalar") == 0) {
        num_found = findkey_scalar(
            reinterpret_cast<const uint8_t*>(mmap_file.data()),
            mmap_file.size(), key_ptrs.data(), key_lens.data(), key_ptrs.size(),
            positions.data(), positions.size(), &status);
    } else if (std::strcmp(algo, "teddy") == 0) {
        num_found = findkey_teddy(
            reinterpret_cast<const uint8_t*>(mmap_file.data()),
            mmap_file.size(), key_ptrs.data(), key_lens.data(), key_ptrs.size(),
            positions.data(), positions.size(), &status);
    } else {
        print_usage_and_exit(argv[0]);
    }

    auto end_time = std::chrono::steady_clock::now();
    const auto duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                              start_time)
            .count();
    const double duration_s = duration_ms / 1000.0;

    const double bytes = mmap_file.size();
    const double mbps =
        duration_s > 0 ? (bytes / (1024.0 * 1024.0)) / duration_s : 0.0;

    switch (status) {
        case FINDKEY_ERR_BAD_ARGS:
            std::fprintf(stderr, "bad arguments");
            return EXIT_FAILURE;
        case FINDKEY_TEDDY_NOT_SUPPORTED:
            std::fprintf(stderr, "teddy not supported by this compiler");
            return EXIT_FAILURE;
        default:
            break;
    }

    std::printf("Algorithm used: %s\n", algo);
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

    std::printf("Time taken: %.2f ms\n", static_cast<double>(duration_ms));
    std::printf("Data size: %.2f MiB\n", bytes / (1024.0 * 1024.0));
    std::printf("Throughput: %.2f MiB/s\n", mbps);

    return 0;
}
