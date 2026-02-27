#pragma once

#include "findkey.h"

#include <cctype>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

static constexpr int DEFAULT_SIGMA = 3;
static constexpr int MAX_SIGMA = DEFAULT_SIGMA + 1;
static constexpr int MAX_GROUPS = 8;  // must be power of 2

struct TeddyCompilationData {
    int sigma = 0;
    int num_groups = 0;
    findkey_teddy_suffix_mode suffix_mode = TEDDY_SUFFIX_RAW;

    // offset from last character to the closing quote
    // 1 for RAW mode
    // 0 for QUOTED mode
    size_t end_quote_offset = 1;

    alignas(16) uint8_t low_table[MAX_SIGMA][16];
    alignas(16) uint8_t high_table[MAX_SIGMA][16];

    std::vector<std::vector<uint32_t>> group_keys;
};

enum candidate_type {
    CANDIDATE_TYPE_MATCH = 0,
    CANDIDATE_BAD_END_QUOTE = 1,
    CANDIDATE_INVALID_QUOTE = 2,
    CANDIDATE_MISSING_COLON = 3,
    CANDIDATE_MISSING_OPEN_QUOTE = 4,
    CANDIDATE_KEY_NOT_FOUND = 5,
};

struct candidate_result {
    candidate_type type = CANDIDATE_TYPE_MATCH;
    size_t position = 0;
    uint32_t key_id = 0;
};

TeddyCompilationData compile_teddy_data(
    const std::vector<std::string_view>& keys,
    enum findkey_teddy_compile_grouping_strategy grouping_strategy,
    enum findkey_teddy_suffix_mode suffix_mode);

bool group_has_exact_suffix(const std::vector<std::string_view>& keys,
                            const TeddyCompilationData& data,
                            uint32_t group,
                            const uint8_t* suffix);

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

static inline candidate_result verify_json_key_candidate(
    const char* str,
    size_t len,
    size_t end_quote,
    size_t max_key_len,
    const std::unordered_map<std::string_view, uint32_t>& key_map) {
    if (end_quote >= len || str[end_quote] != '"') {
        return {CANDIDATE_BAD_END_QUOTE, 0, 0};
    }

    if (!is_valid_quote(str, end_quote)) {
        return {CANDIDATE_INVALID_QUOTE, 0, 0};
    }

    size_t j = end_quote + 1;
    while (j < len && std::isspace(static_cast<unsigned char>(str[j]))) {
        ++j;
    }

    if (j >= len || str[j] != ':') {
        return {CANDIDATE_MISSING_COLON, 0, 0};
    }

    // find opening quote
    size_t start_quote = std::string_view::npos;
    size_t min_start_quote = 0;
    if (end_quote > max_key_len + 1) {
        min_start_quote = end_quote - max_key_len - 1;
    }
    for (size_t k = end_quote - 1; k >= min_start_quote; --k) {
        if (str[k] == '"' && is_valid_quote(str, k)) {
            start_quote = k;
            break;
        }
        if (k == 0) {
            break;
        }
    }
    if (start_quote == std::string_view::npos) {
        return {CANDIDATE_MISSING_OPEN_QUOTE, 0, 0};
    }

    std::string_view sv(str + start_quote + 1, end_quote - start_quote - 1);
    auto it = key_map.find(sv);
    if (it == key_map.end()) {
        return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
    }

    return {CANDIDATE_TYPE_MATCH, start_quote + 1, it->second};
}
