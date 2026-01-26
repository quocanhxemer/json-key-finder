#include "matcher_teddy_baseline.h"
#include "teddy/teddy_common.h"

#include <cctype>
#include <cstring>
#include <string_view>
#include <unordered_map>
#include <vector>

std::vector<findkey_result> matcher_teddy_baseline(
    std::string_view data,
    const std::vector<std::string_view>& keys) {
    std::vector<findkey_result> results;
    results.reserve(1024);  // rough estimate

    const TeddyCompilationData teddy_data = compile_teddy_data(keys);

    // shouldn't happen
    if (teddy_data.sigma <= 0 || teddy_data.num_groups <= 0) {
        return results;
    }

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

    const uint8_t group_mask =
        static_cast<char>(1u << teddy_data.num_groups) - 1u;

    for (size_t position = teddy_data.sigma - 1; position < len; ++position) {
        uint8_t shift_or = 0;

        for (int i = 0; i < teddy_data.sigma; ++i) {
            const uint8_t c =
                static_cast<uint8_t>(str[position - teddy_data.sigma + 1 + i]);
            const uint8_t low_nibble = c & 0x0F;
            const uint8_t high_nibble = (c >> 4) & 0x0F;

            shift_or |= teddy_data.low_table[i][low_nibble] |
                        teddy_data.high_table[i][high_nibble];
        }

        const uint8_t hits = ~shift_or & group_mask;

        if (!hits) {
            continue;
        }

        const size_t end_quote = position + 1;
        if (end_quote >= len || str[end_quote] != '"') {
            continue;
        }

        if (!is_valid_quote(str, end_quote)) {
            continue;
        }

        size_t j = end_quote + 1;
        while (j < len && std::isspace(static_cast<unsigned char>(str[j]))) {
            ++j;
        }

        if (j < len && str[j] == ':') {
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
                continue;
            }

            std::string_view sv(str + start_quote + 1,
                                end_quote - start_quote - 1);
            auto it = key_map.find(sv);
            if (it != key_map.end()) {
                results.push_back(findkey_result{start_quote + 1, it->second});
            }
        }
    }

    return results;
}
