#include "teddy_compile.h"

#include "core/findkey_error.h"
#include "teddy/teddy_grouping.h"

#include <utility>

static void build_teddy_compilation_data_tables(TeddyCompilationData& data) {
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

            for (uint32_t suffix_id : data.group_suffix_ids[group]) {
                const uint8_t c = data.suffixes[suffix_id][i];

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
    TeddySuffixSet suffixes = prepare_teddy_suffixes(keys, config);
    return compile_teddy_data(std::move(suffixes), config.grouping_strategy);
}

TeddyCompilationData compile_teddy_data(
    TeddySuffixSet suffixes,
    findkey_teddy_compile_grouping_strategy grouping_strategy) {
    TeddyCompilationData data{};

    if (suffixes.sigma <= 0 || suffixes.sigma > FINDKEY_TEDDY_MAX_SIGMA) {
        throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                           "Compiled Teddy suffix length is out of range");
    }
    if (suffixes.data.empty()) {
        throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                           "Teddy requires at least one suffix");
    }
    if (suffixes.end_quote_offset > 1) {
        throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                           "Invalid Teddy end quote offset");
    }

    data.sigma = suffixes.sigma;
    data.end_quote_offset = suffixes.end_quote_offset;
    data.suffixes = std::move(suffixes.data);
    data.group_suffix_ids =
        build_teddy_groups(data.suffixes, grouping_strategy, data.sigma);

    data.num_groups = static_cast<int>(data.group_suffix_ids.size());

    build_teddy_compilation_data_tables(data);

    return data;
}

TeddyCompilationMetadata get_teddy_compilation_metadata(
    const TeddyCompilationData& data) {
    std::vector<uint64_t> group_scores;
    uint64_t total_score = 0;
    if (!data.group_suffix_ids.empty() && data.sigma > 0) {
        group_scores.reserve(data.group_suffix_ids.size());
        for (size_t group = 0; group < data.group_suffix_ids.size(); ++group) {
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
        .group_scores = std::move(group_scores),
        .total_score = total_score,
    };
}
