#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

static constexpr int DEFAULT_SIGMA = 3;
static constexpr int MAX_SIGMA = 4;
static constexpr int MAX_GROUPS = 8;  // must be power of 2

struct TeddyCompilationData {
    int sigma = 0;
    int num_groups = 0;

    alignas(16) uint8_t low_table[MAX_SIGMA][16];
    alignas(16) uint8_t high_table[MAX_SIGMA][16];

    std::vector<std::vector<uint32_t>> group_keys;
};

TeddyCompilationData compile_teddy_data(
    const std::vector<std::string_view>& keys);

// quote with even number of backlashes
static inline bool is_valid_quote(const char* str, size_t pos) {
    size_t backslash_count = 0;
    for (size_t k = pos; k > 0; --k) {
        if (str[k - 1] == '\\') {
            ++backslash_count;
        } else {
            break;
        }
    }
    return (backslash_count % 2) == 0;
}
