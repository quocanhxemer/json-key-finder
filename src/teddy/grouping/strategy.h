#pragma once

#include "findkey.h"

#include <algorithm>
#include <array>

namespace teddy::grouping {

inline constexpr std::array SCORE_BASED_STRATEGIES = {
    TEDDY_COMPILE_GREEDY_PAPER_POLICY,
    TEDDY_COMPILE_GREEDY_MIN_DELTA,
    TEDDY_COMPILE_SORTED_SUFFIX_OPTIMAL_PARTITION,
};

[[nodiscard]] constexpr bool grouping_strategy_uses_score(
    findkey_teddy_compile_grouping_strategy strategy) noexcept {
    return std::find(SCORE_BASED_STRATEGIES.begin(),
                     SCORE_BASED_STRATEGIES.end(),
                     strategy) != SCORE_BASED_STRATEGIES.end();
}

}  // namespace teddy::grouping
