#pragma once

#include "findkey.h"
#include "teddy/teddy_suffix.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

static constexpr int MAX_GROUPS = 8;  // must be power of 2

struct TeddyCompilationData {
    int sigma = 0;
    int num_groups = 0;

    // offset from last character to the closing quote
    // 1 for RAW mode
    // 0 for QUOTED mode
    size_t end_quote_offset = 1;

    alignas(16) uint8_t low_table[FINDKEY_TEDDY_MAX_SIGMA][16] = {};
    alignas(16) uint8_t high_table[FINDKEY_TEDDY_MAX_SIGMA][16] = {};

    std::vector<TeddySuffix> suffixes;
    std::vector<std::vector<uint32_t>> group_suffix_ids;
};

struct TeddyCompilationMetadata {
    int sigma = 0;
    int num_groups = 0;
    std::vector<uint64_t> group_scores;
    uint64_t total_score = 0;
};

TeddyCompilationData compile_teddy_data(
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config);

TeddyCompilationData compile_teddy_data(
    TeddySuffixSet suffixes,
    findkey_teddy_compile_grouping_strategy grouping_strategy);

TeddyCompilationMetadata get_teddy_compilation_metadata(
    const TeddyCompilationData& data);
