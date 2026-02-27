#include "matcher_teddy_baseline.h"
#include "teddy/teddy_common.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string_view>
#include <unordered_map>
#include <vector>

template <int Sigma, bool CollectStats>
std::vector<findkey_result> matcher_teddy_baseline_impl(
    std::string_view data,
    const std::vector<std::string_view>& keys,
    const TeddyCompilationData& teddy_data,
    struct findkey_teddy_stats* stats) {
    std::vector<findkey_result> results;
    results.reserve(1024);  // rough estimate

    // hash map for fast key verification
    std::unordered_map<std::string_view, uint32_t> key_map;
    key_map.reserve(keys.size());

    size_t max_key_len = 0;
    for (uint32_t i = 0; i < keys.size(); ++i) {
        max_key_len = std::max(max_key_len, keys[i].size());
        if (key_map.find(keys[i]) == key_map.end()) {
            key_map[keys[i]] = i;
        }
    }

    const char* str = data.data();
    const size_t len = data.size();

    const uint8_t group_mask = (1u << teddy_data.num_groups) - 1u;

    for (size_t position = Sigma - 1; position < len; ++position) {
        uint8_t shift_or = 0;

        for (int i = 0; i < Sigma; ++i) {
            const uint8_t c =
                static_cast<uint8_t>(str[position - Sigma + 1 + i]);
            const uint8_t low_nibble = c & 0x0F;
            const uint8_t high_nibble = (c >> 4) & 0x0F;

            shift_or |= teddy_data.low_table[i][low_nibble] |
                        teddy_data.high_table[i][high_nibble];
        }

        const uint8_t hits = ~shift_or & group_mask;

        if (!hits) {
            continue;
        }

        bool any_exact_suffix = false;
        if constexpr (CollectStats) {
            if (stats) {
                ++stats->prefilter_hit_lanes;
                stats->prefilter_hit_groups += __builtin_popcount(hits);

                uint8_t suffix[Sigma];
                for (int i = 0; i < Sigma; ++i) {
                    suffix[i] =
                        static_cast<uint8_t>(str[position - Sigma + 1 + i]);
                }

                uint8_t group_hits = hits;
                while (group_hits) {
                    const uint32_t group = __builtin_ctz(group_hits);
                    group_hits &= group_hits - 1;

                    if (group_has_exact_suffix(keys, teddy_data, group,
                                               suffix)) {
                        any_exact_suffix = true;
                    } else {
                        ++stats->fp_type1_groups;
                    }
                }
            }
        }

        const size_t end_quote = position + teddy_data.end_quote_offset;
        const candidate_result cr = verify_json_key_candidate(
            str, len, end_quote, max_key_len, key_map);

        if (cr.type == CANDIDATE_TYPE_MATCH) {
            results.push_back({cr.position, cr.key_id});
            if constexpr (CollectStats) {
                if (stats) {
                    ++stats->exact_matches;
                }
            }
        } else {
            if constexpr (CollectStats) {
                if (stats) {
                    switch (cr.type) {
                        case CANDIDATE_BAD_END_QUOTE:
                            ++stats->reject_bad_end_quote;
                            break;
                        case CANDIDATE_INVALID_QUOTE:
                            ++stats->reject_invalid_quote;
                            break;
                        case CANDIDATE_MISSING_COLON:
                            ++stats->reject_missing_colon;
                            break;
                        case CANDIDATE_MISSING_OPEN_QUOTE:
                            ++stats->reject_missing_open_quote;
                            break;
                        case CANDIDATE_KEY_NOT_FOUND:
                            ++stats->reject_key_not_found;
                            break;
                        default:
                            break;
                    }
                    if (any_exact_suffix) {
                        ++stats->fp_type2_lanes;
                    } else {
                        ++stats->fp_type1_lanes;
                    }
                }
            }
        }
    }

    return results;
}

template <bool CollectStats>
std::vector<findkey_result> matcher_teddy_baseline_dispatch(
    std::string_view data,
    const std::vector<std::string_view>& keys,
    enum findkey_teddy_compile_grouping_strategy grouping_strategy,
    enum findkey_teddy_suffix_mode suffix_mode,
    struct findkey_teddy_stats* stats) {
    const TeddyCompilationData teddy_data =
        compile_teddy_data(keys, grouping_strategy, suffix_mode);

    // shouldn't happen
    if (teddy_data.sigma <= 0 || teddy_data.num_groups <= 0) {
        return {};
    }

    switch (teddy_data.sigma) {
        case 1:
            return matcher_teddy_baseline_impl<1, CollectStats>(
                data, keys, teddy_data, stats);
        case 2:
            return matcher_teddy_baseline_impl<2, CollectStats>(
                data, keys, teddy_data, stats);
        case 3:
            return matcher_teddy_baseline_impl<3, CollectStats>(
                data, keys, teddy_data, stats);
        case 4:
            return matcher_teddy_baseline_impl<4, CollectStats>(
                data, keys, teddy_data, stats);
        default:
            return {};
    }
}

std::vector<findkey_result> matcher_teddy_baseline(
    std::string_view data,
    const std::vector<std::string_view>& keys,
    enum findkey_teddy_compile_grouping_strategy grouping_strategy,
    enum findkey_teddy_suffix_mode suffix_mode) {
    return matcher_teddy_baseline_dispatch<false>(data, keys, grouping_strategy,
                                                  suffix_mode, nullptr);
}

std::vector<findkey_result> matcher_teddy_baseline_collect_stats(
    std::string_view data,
    const std::vector<std::string_view>& keys,
    enum findkey_teddy_compile_grouping_strategy grouping_strategy,
    enum findkey_teddy_suffix_mode suffix_mode,
    struct findkey_teddy_stats* stats) {
    return matcher_teddy_baseline_dispatch<true>(data, keys, grouping_strategy,
                                                 suffix_mode, stats);
}
