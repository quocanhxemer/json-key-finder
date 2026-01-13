#include "matcher_teddy.h"

#if COMPILER_SUPPORTS_TEDDY

#include <tmmintrin.h>

#include <cctype>
#include <cstdint>
#include <cstring>

#include <algorithm>
#include <array>
#include <vector>

static constexpr int DEFAULT_SIGMA = 3;
static constexpr int MAX_SIGMA = 4;
static constexpr int MAX_GROUPS = 8;  // must be power of 2

struct TeddyCompilationData {
    int sigma = 0;
    int num_groups = 0;

    alignas(16) uint8_t low_table[MAX_SIGMA][16];
    alignas(16) uint8_t high_table[MAX_SIGMA][16];

    std::vector<std::vector<uint32_t>> group_keys;
};

static TeddyCompilationData compile_teddy_data(
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

// shift current vector to right by offset, fill left with prev vector
// because the compiler complains "the last argument must be an 8-bit immediate"
//                                 ╰（‵□′）╯
static inline __m128i shift_right(__m128i current, __m128i prev, int offset) {
    switch (offset) {
        case 1:
            return _mm_alignr_epi8(current, prev, 15);
        case 2:
            return _mm_alignr_epi8(current, prev, 14);
        case 3:
            return _mm_alignr_epi8(current, prev, 13);
        case 4:
            return _mm_alignr_epi8(current, prev, 12);
        default:
            return current;
    }
}

std::vector<findkey_result> matcher_teddy(
    std::string_view data,
    const std::vector<std::string_view>& keys) {
    std::vector<findkey_result> results;
    results.reserve(1024);  // rough estimate

    const TeddyCompilationData teddy_data = compile_teddy_data(keys);

    // shouldn't happen
    if (teddy_data.sigma <= 0 || teddy_data.num_groups <= 0) {
        return results;
    }

    const char* str = data.data();
    const size_t len = data.size();

    __m128i low_vector[MAX_SIGMA]{};
    __m128i high_vector[MAX_SIGMA]{};
    for (int i = 0; i < teddy_data.sigma; ++i) {
        low_vector[i] = _mm_loadu_si128(
            reinterpret_cast<const __m128i*>(teddy_data.low_table[i]));
        high_vector[i] = _mm_loadu_si128(
            reinterpret_cast<const __m128i*>(teddy_data.high_table[i]));
    }

    // for when the found key isn't alligned
    // doesn't work for sigma > 16 (it should also not be)
    __m128i prev_V[MAX_SIGMA]{};
    for (int i = 0; i < teddy_data.sigma; ++i) {
        prev_V[i] = _mm_set1_epi8(-1);  // 0xFF
    }

    alignas(16) uint8_t match_bytes[16];

    const __m128i mask_0f = _mm_set1_epi8(0x0F);
    const __m128i group_mask_vector =
        _mm_set1_epi8(static_cast<char>(1u << teddy_data.num_groups) - 1u);

    size_t base = 0;
    for (; base < len; base += 16) {
        __m128i bytes;
        if (base + 16 > len) {
            alignas(16) unsigned char chunk[16];
            // dummy fill, 0xF reduces false positives
            std::memset(chunk, 0xFF, 16);
            std::memcpy(chunk, str + base, len - base);
            bytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(chunk));
        } else {
            bytes =
                _mm_loadu_si128(reinterpret_cast<const __m128i*>(str + base));
        }

        const __m128i low_nibbles = _mm_and_si128(bytes, mask_0f);
        const __m128i high_nibbles =
            _mm_and_si128(_mm_srli_epi16(bytes, 4), mask_0f);

        //  (1) load σ vectors for each character from the transition table
        __m128i V[MAX_SIGMA]{};
        for (int i = 0; i < teddy_data.sigma; ++i) {
            const __m128i a = _mm_shuffle_epi8(low_vector[i], low_nibbles);
            const __m128i b = _mm_shuffle_epi8(high_vector[i], high_nibbles);
            V[i] = _mm_or_si128(a, b);
        }

        //  (2) Shift σ − 1 vectors for Bit-Or
        __m128i shift_or = V[teddy_data.sigma - 1];
        for (int i = 0; i < teddy_data.sigma - 1; ++i) {
            const int shift_offset = teddy_data.sigma - 1 - i;
            // shift V to the right, fill left blank spaces with prev_V
            const __m128i shifted_V =
                shift_right(V[i], prev_V[i], shift_offset);
            shift_or = _mm_or_si128(shift_or, shifted_V);
        }

        //  (3) Bit-Or vectors σ − 1 times
        __m128i match = _mm_andnot_si128(shift_or, group_mask_vector);
        _mm_store_si128(reinterpret_cast<__m128i*>(match_bytes), match);

        for (int i = 0; i < 16; ++i) {
            if (base + i >= len) {
                break;
            }

            uint8_t hits = match_bytes[i];
            // extract hit groups
            while (hits) {
                const int group = __builtin_ctz(hits);
                hits &= hits - 1;

                // verify keys in this group
                for (uint32_t key_id : teddy_data.group_keys[group]) {
                    const std::string_view key = keys[key_id];

                    if (base + i + 1 < key.size()) {
                        continue;
                    }

                    const size_t key_start = base + i + 1 - key.size();

                    // no space for quotes
                    if (key_start == 0 || key_start + key.size() >= len) {
                        continue;
                    }
                    if (str[key_start - 1] != '"' ||
                        str[key_start + key.size()] != '"') {
                        continue;
                    }

                    if (!is_valid_quote(str, key_start - 1) ||
                        !is_valid_quote(str, key_start + key.size())) {
                        continue;
                    }

                    if (std::memcmp(str + key_start, key.data(), key.size())) {
                        continue;
                    }

                    // colon
                    size_t j = key_start + key.size() + 1;
                    while (j < len &&
                           std::isspace(static_cast<unsigned char>(str[j]))) {
                        ++j;
                    }
                    if (j < len && str[j] == ':') {
                        results.push_back(findkey_result{key_start, key_id});
                    }
                }
            }
        }

        for (int i = 0; i < teddy_data.sigma; ++i) {
            prev_V[i] = V[i];
        }
    }

    return results;
}

#else

std::vector<findkey_result> matcher_teddy(
    std::string_view data,
    const std::vector<std::string_view>& keys) {
    (void)data;
    (void)keys;
    return {};
}

#endif
