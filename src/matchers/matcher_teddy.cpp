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

template <int Sigma>
std::vector<findkey_result> matcher_teddy_impl(
    std::string_view data,
    const TeddyCompilationData& teddy_data) {
    std::vector<findkey_result> results;
    results.reserve(1024);  // rough estimate

    const char* str = data.data();
    const size_t len = data.size();

    __m128i low_vector[Sigma]{};
    __m128i high_vector[Sigma]{};
    for (int i = 0; i < Sigma; ++i) {
        low_vector[i] = _mm_loadu_si128(
            reinterpret_cast<const __m128i*>(teddy_data.low_table[i]));
        high_vector[i] = _mm_loadu_si128(
            reinterpret_cast<const __m128i*>(teddy_data.high_table[i]));
    }

    // for when the found key isn't alligned
    __m128i prev_V[Sigma]{};
    for (int i = 0; i < Sigma; ++i) {
        prev_V[i] = _mm_set1_epi8(-1);  // 0xFF
    }

    const __m128i mask_0f = _mm_set1_epi8(0x0F);
    const __m128i group_mask_vector =
        _mm_set1_epi8(static_cast<char>((1u << teddy_data.num_groups) - 1u));
    const __m128i zero_vector = _mm_setzero_si128();

    for (size_t base = 0; base < len; base += 16) {
        __m128i bytes;
        if (base + 16 > len) {
            alignas(16) unsigned char chunk[16];
            // dummy fill, 0xF reduces false positives
            std::memset(chunk, 0xFF, sizeof(chunk));
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
        __m128i V[Sigma]{};
        for (int i = 0; i < Sigma; ++i) {
            const __m128i a = _mm_shuffle_epi8(low_vector[i], low_nibbles);
            const __m128i b = _mm_shuffle_epi8(high_vector[i], high_nibbles);
            V[i] = _mm_or_si128(a, b);
        }

        //  (2) Shift σ − 1 vectors for Bit-Or
        __m128i shift_or = V[Sigma - 1];
        for (int i = 0; i < Sigma - 1; ++i) {
            const int shift_offset = Sigma - 1 - i;
            // shift V to the right, fill left blank spaces with prev_V
            const __m128i shifted_V =
                _mm_alignr_epi8(V[i], prev_V[i], 16 - shift_offset);
            //  (3) Bit-Or vectors σ − 1 times
            shift_or = _mm_or_si128(shift_or, shifted_V);
        }

        __m128i match = _mm_andnot_si128(shift_or, group_mask_vector);
        __m128i is_zero = _mm_cmpeq_epi8(match, zero_vector);
        uint16_t hit_mask = ~static_cast<uint16_t>(_mm_movemask_epi8(is_zero));

        while (hit_mask) {
            const int i = __builtin_ctz(hit_mask);
            hit_mask &= hit_mask - 1;

            if (base + i >= len) {
                break;
            }

            const size_t last_char = base + i;
            const size_t end_quote = last_char + teddy_data.end_quote_offset;

            const candidate_result cr =
                verify_json_key_candidate(str, len, end_quote, teddy_data.dfa);
            if (cr.type == CANDIDATE_TYPE_MATCH) {
                results.push_back({cr.position, cr.key_id});
            }
        }

        for (int i = 0; i < Sigma; ++i) {
            prev_V[i] = V[i];
        }
    }

    return results;
}

std::vector<findkey_result> matcher_teddy(
    std::string_view data,
    const std::vector<std::string_view>& keys,
    enum findkey_teddy_compile_grouping_strategy grouping_strategy,
    enum findkey_teddy_suffix_mode suffix_mode) {
    const TeddyCompilationData teddy_data =
        compile_teddy_data(keys, grouping_strategy, suffix_mode);

    // shouldn't happen
    if (teddy_data.sigma <= 0 || teddy_data.num_groups <= 0) {
        return {};
    }

    switch (teddy_data.sigma) {
        case 1:
            return matcher_teddy_impl<1>(data, teddy_data);
        case 2:
            return matcher_teddy_impl<2>(data, teddy_data);
        case 3:
            return matcher_teddy_impl<3>(data, teddy_data);
        case 4:
            return matcher_teddy_impl<4>(data, teddy_data);
        default:
            return {};
    }
}

#else

std::vector<findkey_result> matcher_teddy(
    std::string_view data,
    const std::vector<std::string_view>& keys,
    enum findkey_teddy_compile_grouping_strategy grouping_strategy,
    enum findkey_teddy_suffix_mode suffix_mode) {
    (void)data;
    (void)keys;
    (void)grouping_strategy;
    (void)suffix_mode;
    return {};
}

#endif
