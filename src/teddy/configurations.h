#pragma once

#include "findkey.h"

#include <array>
#include <span>
#include <vector>

namespace teddy {

inline constexpr std::array ALL_GROUPING_STRATEGIES = {
    TEDDY_COMPILE_GREEDY_PAPER_POLICY,
    TEDDY_COMPILE_GREEDY_MIN_DELTA,
    TEDDY_COMPILE_HASH_STD,
    TEDDY_COMPILE_HASH_ADLER32,
    TEDDY_COMPILE_HASH_CRC32,
    TEDDY_COMPILE_HASH_XXHASH,
    TEDDY_COMPILE_HASH_FNV1A,
    TEDDY_COMPILE_SORTED_SUFFIX_ROUND_ROBIN,
    TEDDY_COMPILE_SORTED_SUFFIX_PARTITION,
    TEDDY_COMPILE_SORTED_SUFFIX_OPTIMAL_PARTITION,
};

inline constexpr std::array ALL_GROUPING_SCORES = {
    TEDDY_GROUPING_SCORE_PAPER,
    TEDDY_GROUPING_SCORE_PAPER_NIBBLE,
    TEDDY_GROUPING_SCORE_NIBBLE_COUNT,
};

inline constexpr std::array ALL_SUFFIX_MODES = {
    TEDDY_SUFFIX_RAW,
    TEDDY_SUFFIX_QUOTED,
};

inline constexpr auto ALL_SIGMAS = [] {
    std::array<int, FINDKEY_TEDDY_MAX_SUFFIX_LENGTH> sigmas{};
    for (size_t i = 0; i < sigmas.size(); ++i) {
        sigmas[i] = static_cast<int>(i) + 1;
    }
    return sigmas;
}();

static_assert(ALL_GROUPING_STRATEGIES.size() ==
              FINDKEY_TEDDY_COMPILE_GROUPING_STRATEGY_COUNT);
static_assert(ALL_GROUPING_SCORES.size() == FINDKEY_TEDDY_GROUPING_SCORE_COUNT);
static_assert(ALL_SUFFIX_MODES.size() == FINDKEY_TEDDY_SUFFIX_MODE_COUNT);

std::vector<findkey_teddy_grouping_config> make_grouping_configurations(
    std::span<const findkey_teddy_compile_grouping_strategy> strategies,
    std::span<const findkey_teddy_grouping_score> scores);

std::vector<findkey_teddy_grouping_config> all_grouping_configurations();

std::vector<findkey_teddy_config> make_teddy_configurations(
    std::span<const findkey_teddy_compile_grouping_strategy> strategies,
    std::span<const findkey_teddy_grouping_score> scores,
    std::span<const findkey_teddy_suffix_mode> suffix_modes,
    std::span<const int> sigmas);

std::vector<findkey_teddy_config> all_teddy_configurations();

}  // namespace teddy
