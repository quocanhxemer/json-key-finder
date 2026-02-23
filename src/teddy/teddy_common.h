#pragma once

#include "findkey.h"

#include <cstdint>
#include <string_view>
#include <vector>

static constexpr int DEFAULT_SIGMA = 3;
static constexpr int MAX_SIGMA = DEFAULT_SIGMA + 1;
static constexpr int MAX_GROUPS = 8;  // must be power of 2

struct TeddyCompilationData {
    int sigma = 0;
    int num_groups = 0;

    // offset from last character to the closing quote
    // default for RAW mode is 1, but for QUOTED mode it can be larger due to
    // escape characters
    size_t end_quote_offset = 1;

    alignas(16) uint8_t low_table[MAX_SIGMA][16];
    alignas(16) uint8_t high_table[MAX_SIGMA][16];

    std::vector<std::vector<uint32_t>> group_keys;
};

TeddyCompilationData compile_teddy_data(
    const std::vector<std::string_view>& keys,
    enum findkey_teddy_compile_grouping_strategy grouping_strategy,
    enum findkey_teddy_suffix_mode suffix_mode);

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
