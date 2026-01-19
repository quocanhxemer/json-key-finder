#include "matcher_teddy.h"

#if COMPILER_SUPPORTS_TEDDY

#include "teddy/teddy_common.h"

#include <tmmintrin.h>

#include <cctype>
#include <cstdint>
#include <cstring>

#include <algorithm>
#include <array>
#include <vector>

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
