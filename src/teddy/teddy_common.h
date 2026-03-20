#pragma once

#include "findkey.h"

#include <array>
#include <cctype>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

static constexpr int DEFAULT_SIGMA = 3;
static constexpr int MAX_SIGMA = DEFAULT_SIGMA + 1;
static constexpr int MAX_GROUPS = 8;  // must be power of 2

struct TrieNode {
    std::array<int32_t, 256> children{};
    int32_t key_id = -1;
    uint8_t group_mask = 0;

    TrieNode() { children.fill(-1); }
};

struct DFA {
    std::vector<TrieNode> nodes;
    size_t max_key_len = 0;
};

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

    DFA dfa;
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
    const DFA& dfa,
    uint8_t candidate_groups) {
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

    int32_t current_node = 0;
    size_t consumed = 0;
    uint8_t active_groups = candidate_groups;

    for (size_t position = end_quote; position > 0;) {
        --position;
        const uint8_t c = static_cast<uint8_t>(str[position]);

        if (c == '"' && is_valid_quote(str, position)) {
            if (dfa.nodes[current_node].key_id != -1) {
                return {CANDIDATE_TYPE_MATCH, position + 1,
                        static_cast<uint32_t>(dfa.nodes[current_node].key_id)};
            }
            return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
        }

        if (consumed >= dfa.max_key_len) {
            return {CANDIDATE_MISSING_OPEN_QUOTE, 0, 0};
        }

        const int32_t next_node = dfa.nodes[current_node].children[c];
        if (next_node == -1) {
            return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
        }

        current_node = next_node;
        active_groups &= dfa.nodes[current_node].group_mask;
        if (!active_groups) {
            return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
        }
        ++consumed;
    }

    return {CANDIDATE_MISSING_OPEN_QUOTE, 0, 0};
}
