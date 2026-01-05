#include "findkey.h"
#include "mmap_file.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define POSITIONS_CAPACITY 1024 * 1024

int main(int argc, char** argv) {
    const char* key = nullptr;
    const char* file = nullptr;

    auto print_usage_and_exit = [&argv]() {
        std::fprintf(stderr, "Usage: %s <key> <file>\n", argv[0]);
        std::exit(EXIT_FAILURE);
    };

    if (argc != 3) {
        print_usage_and_exit();
    }

    for (int i = 1; i < argc; ++i) {
        if (!key) {
            key = argv[i];
        } else if (!file) {
            file = argv[i];
        } else {
            print_usage_and_exit();
        }
    }

    MMapFile mmap_file(file);

    std::vector<size_t> positions(POSITIONS_CAPACITY);
    int status = 0;

    size_t num_found = findkey_scalar(
        reinterpret_cast<const uint8_t*>(mmap_file.data()), mmap_file.size(),
        reinterpret_cast<const uint8_t*>(key), std::strlen(key),
        positions.data(), positions.size(), &status);

    if (status != FINDKEY_OK) {
        std::fprintf(stderr, "bad arguments");
        return EXIT_FAILURE;
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
