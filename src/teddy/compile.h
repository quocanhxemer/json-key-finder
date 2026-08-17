#pragma once

#include "findkey.h"
#include "teddy/suffix.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace teddy {

inline constexpr int MAX_GROUPS = 8;

static_assert(MAX_GROUPS > 0 && (MAX_GROUPS & (MAX_GROUPS - 1)) == 0,
              "Teddy group count must be a positive power of two");

struct CompilationData {
    int sigma = 0;
    int num_groups = 0;

    // offset from last character to the closing quote
    // 1 for RAW mode
    // 0 for QUOTED mode
    size_t end_quote_offset = 1;

    alignas(16) uint8_t low_table[FINDKEY_TEDDY_MAX_SIGMA][16] = {};
    alignas(16) uint8_t high_table[FINDKEY_TEDDY_MAX_SIGMA][16] = {};

    std::vector<Suffix> suffixes;
    std::vector<std::vector<uint32_t>> group_suffix_ids;
};

struct CompilationMetadata {
    int sigma = 0;
    int num_groups = 0;
    std::vector<uint64_t> group_scores;
    uint64_t total_score = 0;
};

CompilationData compile(const std::vector<std::string_view>& keys,
                        const findkey_teddy_config& config);

CompilationData compile(
    SuffixSet suffixes,
    findkey_teddy_compile_grouping_strategy grouping_strategy);

CompilationMetadata get_compilation_metadata(const CompilationData& data);

}  // namespace teddy
