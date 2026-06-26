#pragma once

#include "teddy/teddy_compile.h"

#include <cctype>
#include <cstdint>
#include <vector>

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

bool group_has_exact_suffix(const std::vector<std::string_view>& keys,
                            const TeddyCompilationData& data,
                            uint32_t group,
                            const uint8_t* suffix);

// quote with even number of backslashes
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
        active_groups &= dfa.node_group_masks[current_node];
        if (!active_groups) {
            return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
        }
        ++consumed;
    }

    return {CANDIDATE_MISSING_OPEN_QUOTE, 0, 0};
}
