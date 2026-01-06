#include "findkey.h"
#include "mmap_file.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

int main(int argc, char** argv) {
    auto print_usage_and_exit = [&argv]() {
        std::fprintf(stderr, "Usage: [--algo scalar|teddy] %s <key> <file>\n",
                     argv[0]);
        std::exit(EXIT_FAILURE);
    };

    const char* algo = "scalar";
    int idx = 1;
    if (argc >= 3 && std::strcmp(argv[1], "--algo") == 0) {
        if (argc != 5) {
            print_usage_and_exit();
        }
        algo = argv[2];
        idx = 3;
    } else if (argc != 3) {
        print_usage_and_exit();
    }
    const char* key = argv[idx++];
    const char* file = argv[idx++];

    MMapFile mmap_file(file);

    constexpr size_t POSITIONS_CAPACITY = 1024 * 1024;
    std::vector<size_t> positions(POSITIONS_CAPACITY);
    int status = 0;

    size_t num_found = 0;
    if (std::strcmp(algo, "scalar") == 0) {
        num_found = findkey_scalar(
            reinterpret_cast<const uint8_t*>(mmap_file.data()),
            mmap_file.size(), reinterpret_cast<const uint8_t*>(key),
            std::strlen(key), positions.data(), positions.size(), &status);
    } else if (std::strcmp(algo, "teddy") == 0) {
        num_found = findkey_teddy(
            reinterpret_cast<const uint8_t*>(mmap_file.data()),
            mmap_file.size(), reinterpret_cast<const uint8_t*>(key),
            std::strlen(key), positions.data(), positions.size(), &status);
    } else {
        print_usage_and_exit();
    }

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

    std::printf("Found %zu occurrences of key \"%s\" in file \"%s\":\n",
                num_found, key, file);
    for (size_t i = 0; i < num_found && i < positions.size(); ++i) {
        std::printf("  Position: %zu\n", positions[i]);
    }
    if (num_found > positions.size()) {
        std::printf("  ... and %zu more\n", num_found - positions.size());
    }

    return 0;
}
