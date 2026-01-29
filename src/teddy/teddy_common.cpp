#include "teddy_common.h"
#include <algorithm>
#include <array>

struct Group {
    std::vector<uint32_t> key_ids;
    uint8_t b[DEFAULT_SIGMA];

    uint32_t score;

    Group(uint32_t key_id, const std::string_view& key, int sigma) {
        key_ids.push_back(key_id);
        score = 1;
        for (int i = 0; i < sigma; ++i) {
            b[i] = static_cast<uint8_t>(key[key.size() - sigma + i]);
            score *= __builtin_popcount(b[i]);
        }
    }

    static uint32_t merge_score(const Group& a, const Group& b, int sigma) {
        uint32_t s = 1;
        for (int i = 0; i < sigma; ++i) {
            uint8_t merged = a.b[i] | b.b[i];
            s *= __builtin_popcount(merged);
        }
        return s;
    }
};

TeddyCompilationData compile_teddy_data(
    const std::vector<std::string_view>& keys) {
    TeddyCompilationData data;

    if (keys.empty()) {
        return data;
    }

    size_t min_len = keys[0].size();
    for (std::string_view key : keys) {
        min_len = std::min(min_len, key.size());
    }

    data.sigma = std::min(static_cast<int>(min_len), DEFAULT_SIGMA);

    // edge case: empty keys should be filtered out earlier
    if (data.sigma <= 0) {
        return data;
    }

    std::vector<Group> groups;
    groups.reserve(keys.size());
    for (uint32_t i = 0; i < keys.size(); ++i) {
        groups.emplace_back(i, keys[i], data.sigma);
    }

    while (groups.size() > MAX_GROUPS) {
        // find best pair to merge
        uint32_t best = UINT32_MAX;
        int best_i = -1;
        int best_j = -1;
        bool perfect_found = false;
        uint32_t merge_score = 0;

        for (size_t i = 0; i < groups.size(); ++i) {
            for (size_t j = i + 1; j < groups.size(); ++j) {
                uint32_t new_score =
                    Group::merge_score(groups[i], groups[j], data.sigma);
                uint32_t old_score = groups[i].score + groups[j].score;
                if (new_score < old_score) {
                    best_i = i;
                    best_j = j;
                    merge_score = new_score;
                    perfect_found = true;
                    break;
                }

                if (best > new_score - old_score) {
                    best = new_score - old_score;
                    best_i = i;
                    best_j = j;
                    merge_score = new_score;
                }
            }
            if (perfect_found) {
                break;
            }
        }

        // shouldn't happen - an iteration should always find a pair to merge
        if (best_i == -1) {
            break;
        }

        Group& target = groups[best_i];
        Group& source = groups[best_j];

        for (int i = 0; i < data.sigma; ++i) {
            target.b[i] |= source.b[i];
        }
        target.key_ids.insert(target.key_ids.end(), source.key_ids.begin(),
                              source.key_ids.end());
        target.score = merge_score;

        groups.erase(groups.begin() + best_j);
    }

    for (auto& group : groups) {
        data.group_keys.push_back(std::move(group.key_ids));
    }

    data.num_groups = static_cast<int>(data.group_keys.size());
    if (!data.num_groups) {
        return data;
    }

    for (int i = 0; i < MAX_SIGMA; ++i) {
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
                const size_t idx = key.size() - data.sigma + i;
                const uint8_t c = static_cast<uint8_t>(key[idx]);

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

    return data;
}
