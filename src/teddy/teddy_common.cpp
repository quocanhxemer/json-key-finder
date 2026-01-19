#include "teddy_common.h"
#include <algorithm>
#include <array>

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

    // partition
    std::array<std::vector<uint32_t>, MAX_GROUPS> buckets;
    for (uint32_t key_id = 0; key_id < keys.size(); ++key_id) {
        std::string_view suffix(
            keys[key_id].data() + keys[key_id].size() - data.sigma, data.sigma);
        const uint32_t hash =
            static_cast<uint32_t>(std::hash<std::string_view>{}(suffix));

        buckets[hash & (MAX_GROUPS - 1)].push_back(key_id);
    }

    // compress into struct
    for (int group = 0; group < MAX_GROUPS; ++group) {
        if (buckets[group].empty()) {
            continue;
        }

        data.group_keys.push_back(std::move(buckets[group]));
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
