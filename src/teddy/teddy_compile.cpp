#include "teddy_compile.h"
#include "teddy/teddy_grouping.h"
#include "teddy/teddy_suffix.h"
#include "teddy/teddy_verify.h"

#include <algorithm>
#include <utility>

bool group_has_exact_suffix(const std::vector<std::string_view>& keys,
                            const TeddyCompilationData& data,
                            uint32_t group,
                            const uint8_t* suffix) {
    for (uint32_t key_id : data.group_keys[group]) {
        bool found = true;
        for (int i = 0; i < data.sigma; ++i) {
            uint8_t suffix_byte = teddy_suffix_byte(keys[key_id], data.sigma, i,
                                                    data.suffix_mode);
            if (suffix_byte != suffix[i]) {
                found = false;
                break;
            }
        }

        if (found) {
            return true;
        }
    }

    return false;
}

static DFA buildDFA(const std::vector<std::string_view>& keys,
                    const std::vector<std::vector<uint32_t>>& group_keys) {
    DFA dfa;
    dfa.nodes.emplace_back();  // root
    dfa.node_group_masks.push_back(0);
    std::vector<uint8_t> key_groups(keys.size(), 0);

    for (uint32_t group = 0; group < group_keys.size(); ++group) {
        const uint8_t group_bit = static_cast<uint8_t>(1u << group);
        for (uint32_t key_id : group_keys[group]) {
            key_groups[key_id] |= group_bit;
        }
    }

    for (uint32_t key_id = 0; key_id < keys.size(); ++key_id) {
        const std::string_view key = keys[key_id];
        const uint8_t key_group_mask = key_groups[key_id];
        dfa.max_key_len = std::max(dfa.max_key_len, key.size());

        int32_t current_node = 0;
        for (size_t i = key.size(); i > 0; --i) {
            const uint8_t c = static_cast<uint8_t>(key[i - 1]);
            if (dfa.nodes[current_node].children[c] == -1) {
                dfa.nodes[current_node].children[c] =
                    static_cast<int32_t>(dfa.nodes.size());
                dfa.nodes.emplace_back();
                dfa.node_group_masks.push_back(0);
            }
            current_node = dfa.nodes[current_node].children[c];
            dfa.node_group_masks[current_node] |= key_group_mask;
        }

        if (dfa.nodes[current_node].key_id != -1) {  // duplicated key
            continue;
        }

        dfa.nodes[current_node].key_id = key_id;
    }

    return dfa;
}

static void build_teddy_compilation_data_tables(
    TeddyCompilationData& data,
    const std::vector<std::string_view>& keys,
    findkey_teddy_suffix_mode suffix_mode) {
    for (int i = 0; i < FINDKEY_TEDDY_MAX_SIGMA; ++i) {
        for (int j = 0; j < 16; ++j) {
            data.low_table[i][j] = 0xFF;
            data.high_table[i][j] = 0xFF;
        }
    }

    for (int i = 0; i < data.sigma; ++i) {
        for (int group = 0; group < data.num_groups; ++group) {
            bool low_filled[16] = {false};
            bool high_filled[16] = {false};

            for (uint32_t key_id : data.group_keys[group]) {
                const std::string_view key = keys[key_id];
                const uint8_t c =
                    teddy_suffix_byte(key, data.sigma, i, suffix_mode);

                uint8_t low_nibble = c & 0x0F;
                uint8_t high_nibble = (c >> 4) & 0x0F;
                low_filled[low_nibble] = true;
                high_filled[high_nibble] = true;
            }

            const uint8_t mask = ~static_cast<uint8_t>(1u << group);
            for (int nibble = 0; nibble < 16; ++nibble) {
                if (low_filled[nibble]) {
                    data.low_table[i][nibble] &= mask;
                }
                if (high_filled[nibble]) {
                    data.high_table[i][nibble] &= mask;
                }
            }
        }
    }
}

TeddyCompilationData compile_teddy_data(
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config) {
    TeddyCompilationData data{};
    data.suffix_mode = config.suffix_mode;

    if (keys.empty()) {
        return {};
    }

    size_t min_len = teddy_virtual_length(keys[0], config.suffix_mode);
    for (std::string_view key : keys) {
        min_len =
            std::min(min_len, teddy_virtual_length(key, config.suffix_mode));
    }

    const int compiled_sigma = (config.suffix_mode == TEDDY_SUFFIX_QUOTED)
                                   ? (config.sigma + 1)
                                   : config.sigma;

    data.sigma = std::min(static_cast<int>(min_len), compiled_sigma);
    data.end_quote_offset = (config.suffix_mode == TEDDY_SUFFIX_QUOTED) ? 0 : 1;

    // edge case: empty keys should be filtered out earlier
    if (data.sigma <= 0) {
        return {};
    }

    data.group_keys = build_teddy_groups(keys, config, data.sigma);

    data.num_groups = static_cast<int>(data.group_keys.size());
    if (!data.num_groups) {
        return {};
    }

    data.dfa = buildDFA(keys, data.group_keys);
    build_teddy_compilation_data_tables(data, keys, config.suffix_mode);

    return data;
}

TeddyCompilationMetadata get_teddy_compilation_metadata(
    const TeddyCompilationData& data) {
    std::vector<uint64_t> group_scores;
    uint64_t total_score = 0;
    if (!data.group_keys.empty() && data.sigma > 0) {
        group_scores.reserve(data.group_keys.size());
        for (size_t group = 0; group < data.group_keys.size(); ++group) {
            const uint8_t group_bit = static_cast<uint8_t>(1u << group);
            uint32_t group_score = 1;

            for (int i = 0; i < data.sigma; ++i) {
                uint8_t merged = 0;
                for (int nibble = 0; nibble < 16; ++nibble) {
                    const bool low_present =
                        (data.low_table[i][nibble] & group_bit) == 0;
                    const bool high_present =
                        (data.high_table[i][nibble] & group_bit) == 0;

                    if (low_present) {
                        merged |= static_cast<uint8_t>(nibble);
                    }
                    if (high_present) {
                        merged |= static_cast<uint8_t>(nibble << 4);
                    }
                }

                group_score *= __builtin_popcount(merged);
            }

            group_scores.push_back(group_score);
            total_score += group_score;
        }
    }

    return {
        .sigma = data.sigma,
        .num_groups = data.num_groups,
        .dfa_nodes = data.dfa.nodes.size(),
        .max_key_len = data.dfa.max_key_len,
        .group_scores = std::move(group_scores),
        .total_score = total_score,
    };
}
