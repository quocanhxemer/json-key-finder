#include "teddy/teddy_grouping.h"

#include "teddy/teddy_compile.h"
#include "teddy/teddy_hash.h"
#include "teddy/teddy_suffix.h"

#include <algorithm>
#include <array>
#include <climits>
#include <numeric>
#include <utility>

struct TeddySuffixGroup {
    std::vector<uint32_t> key_ids;
    std::array<uint8_t, FINDKEY_TEDDY_MAX_SIGMA> b{};

    uint32_t score = 1;

    TeddySuffixGroup(uint32_t key_id,
                     const std::string_view& key,
                     int sigma,
                     enum findkey_teddy_suffix_mode suffix_mode) {
        key_ids.push_back(key_id);
        for (int i = 0; i < sigma; ++i) {
            b[i] = teddy_suffix_byte(key, sigma, i, suffix_mode);
            score *= __builtin_popcount(b[i]);
        }
    }

    static uint32_t merge_score(const TeddySuffixGroup& a,
                                const TeddySuffixGroup& b,
                                int sigma) {
        uint32_t s = 1;
        for (int i = 0; i < sigma; ++i) {
            uint8_t merged = a.b[i] | b.b[i];
            s *= __builtin_popcount(merged);
        }
        return s;
    }

    void merge_from(const TeddySuffixGroup& source,
                    int sigma,
                    uint32_t merged_score) {
        for (int i = 0; i < sigma; ++i) {
            b[i] |= source.b[i];
        }

        key_ids.reserve(key_ids.size() + source.key_ids.size());
        key_ids.insert(key_ids.end(), source.key_ids.begin(),
                       source.key_ids.end());
        score = merged_score;
    }
};

static std::vector<std::vector<uint32_t>> build_greedy_groups(
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config,
    int sigma,
    bool paper_early_exit) {
    std::vector<TeddySuffixGroup> groups;
    groups.reserve(keys.size());
    for (uint32_t i = 0; i < keys.size(); ++i) {
        groups.emplace_back(i, keys[i], sigma, config.suffix_mode);
    }

    while (groups.size() > MAX_GROUPS) {
        // find best pair to merge
        int best = INT_MAX;
        int best_i = -1;
        int best_j = -1;
        uint32_t selected_merge_score = 0;

        for (size_t i = 0; i < groups.size(); ++i) {
            for (size_t j = i + 1; j < groups.size(); ++j) {
                int new_score =
                    TeddySuffixGroup::merge_score(groups[i], groups[j], sigma);
                int old_score = static_cast<int>(groups[i].score) +
                                static_cast<int>(groups[j].score);
                if (paper_early_exit && new_score < old_score) {
                    best_i = i;
                    best_j = j;
                    selected_merge_score = static_cast<uint32_t>(new_score);
                    break;
                }

                if (best > new_score - old_score) {
                    best = new_score - old_score;
                    best_i = i;
                    best_j = j;
                    selected_merge_score = static_cast<uint32_t>(new_score);
                }
            }
        }

        // shouldn't happen - an iteration should always find a pair to merge
        if (best_i == -1) {
            break;
        }

        TeddySuffixGroup& target = groups[best_i];
        TeddySuffixGroup& source = groups[best_j];
        target.merge_from(source, sigma, selected_merge_score);

        groups.erase(groups.begin() + best_j);
    }

    std::vector<std::vector<uint32_t>> group_keys;
    group_keys.reserve(groups.size());
    for (auto& group : groups) {
        group_keys.push_back(std::move(group.key_ids));
    }
    return group_keys;
}

static std::vector<std::vector<uint32_t>> build_hash_groups(
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config,
    int sigma) {
    // partition
    std::array<std::vector<uint32_t>, MAX_GROUPS> buckets;
    for (uint32_t key_id = 0; key_id < keys.size(); ++key_id) {
        uint8_t suffix_buf[FINDKEY_TEDDY_MAX_SIGMA];
        for (int i = 0; i < sigma; ++i) {
            suffix_buf[i] =
                teddy_suffix_byte(keys[key_id], sigma, i, config.suffix_mode);
        }
        const uint32_t hash =
            hash_teddy_suffix(suffix_buf, sigma, config.grouping_strategy);

        buckets[hash & (MAX_GROUPS - 1)].push_back(key_id);
    }

    // compress into struct
    std::vector<std::vector<uint32_t>> group_keys;
    group_keys.reserve(MAX_GROUPS);
    for (int group = 0; group < MAX_GROUPS; ++group) {
        if (buckets[group].empty()) {
            continue;
        }

        group_keys.push_back(std::move(buckets[group]));
    }
    return group_keys;
}

static std::vector<uint32_t> sorted_key_ids_by_suffix(
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config,
    int sigma) {
    std::vector<uint32_t> sorted_key_ids(keys.size());
    std::iota(sorted_key_ids.begin(), sorted_key_ids.end(), uint32_t{0});

    std::sort(
        sorted_key_ids.begin(), sorted_key_ids.end(),
        [&](uint32_t left_key_id, uint32_t right_key_id) {
            for (int suffix_index = 0; suffix_index < sigma; ++suffix_index) {
                const uint8_t left_byte = teddy_suffix_byte(
                    keys[left_key_id], sigma, suffix_index, config.suffix_mode);
                const uint8_t right_byte =
                    teddy_suffix_byte(keys[right_key_id], sigma, suffix_index,
                                      config.suffix_mode);
                if (left_byte != right_byte) {
                    return left_byte < right_byte;
                }
            }
            return left_key_id < right_key_id;
        });

    return sorted_key_ids;
}

static std::vector<std::vector<uint32_t>>
build_sorted_suffix_round_robin_groups(
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config,
    int sigma) {
    const std::vector<uint32_t> sorted_key_ids =
        sorted_key_ids_by_suffix(keys, config, sigma);
    const size_t num_groups =
        std::min(sorted_key_ids.size(), static_cast<size_t>(MAX_GROUPS));

    std::vector<std::vector<uint32_t>> group_keys(num_groups);
    for (size_t key_index = 0; key_index < sorted_key_ids.size(); ++key_index) {
        group_keys[key_index % num_groups].push_back(sorted_key_ids[key_index]);
    }
    return group_keys;
}

static std::vector<std::vector<uint32_t>> build_sorted_suffix_partition_groups(
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config,
    int sigma) {
    const std::vector<uint32_t> sorted_key_ids =
        sorted_key_ids_by_suffix(keys, config, sigma);
    const size_t num_groups =
        std::min(sorted_key_ids.size(), static_cast<size_t>(MAX_GROUPS));

    std::vector<std::vector<uint32_t>> group_keys;
    group_keys.reserve(num_groups);
    for (size_t group_index = 0; group_index < num_groups; ++group_index) {
        const size_t begin_index =
            group_index * sorted_key_ids.size() / num_groups;
        const size_t end_index =
            (group_index + 1) * sorted_key_ids.size() / num_groups;

        std::vector<uint32_t> group;
        group.reserve(end_index - begin_index);
        for (size_t key_index = begin_index; key_index < end_index;
             ++key_index) {
            group.push_back(sorted_key_ids[key_index]);
        }
        group_keys.push_back(std::move(group));
    }
    return group_keys;
}

std::vector<std::vector<uint32_t>> build_teddy_groups(
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config,
    int sigma) {
    switch (config.grouping_strategy) {
        case TEDDY_COMPILE_PAPER_GREEDY:
            return build_greedy_groups(keys, config, sigma, true);
        case TEDDY_COMPILE_PAPER_IMPROVED_GREEDY:
            return build_greedy_groups(keys, config, sigma, false);
        case TEDDY_COMPILE_HASH_STD:
        case TEDDY_COMPILE_HASH_ADLER32:
        case TEDDY_COMPILE_HASH_CRC32:
        case TEDDY_COMPILE_HASH_XXHASH:
        case TEDDY_COMPILE_HASH_FNV1A:
            return build_hash_groups(keys, config, sigma);
        case TEDDY_COMPILE_SORTED_SUFFIX_ROUND_ROBIN:
            return build_sorted_suffix_round_robin_groups(keys, config, sigma);
        case TEDDY_COMPILE_SORTED_SUFFIX_PARTITION:
            return build_sorted_suffix_partition_groups(keys, config, sigma);
        default:
            return {};
    }
}
