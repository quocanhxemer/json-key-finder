#include "teddy/configurations.h"

#include "teddy/grouping/strategy.h"

namespace teddy {

std::vector<findkey_teddy_grouping_config> make_grouping_configurations(
    std::span<const findkey_teddy_compile_grouping_strategy> strategies,
    std::span<const findkey_teddy_grouping_score> scores) {
    std::vector<findkey_teddy_grouping_config> configurations;
    configurations.reserve(strategies.size() * scores.size());

    for (const auto strategy : strategies) {
        if (grouping::grouping_strategy_uses_score(strategy)) {
            for (const auto score : scores) {
                configurations.push_back({strategy, score});
            }
            continue;
        }
        configurations.push_back({strategy, TEDDY_GROUPING_SCORE_PAPER});
    }

    return configurations;
}

std::vector<findkey_teddy_grouping_config> all_grouping_configurations() {
    return make_grouping_configurations(ALL_GROUPING_STRATEGIES,
                                        ALL_GROUPING_SCORES);
}

std::vector<findkey_teddy_config> make_teddy_configurations(
    std::span<const findkey_teddy_compile_grouping_strategy> strategies,
    std::span<const findkey_teddy_grouping_score> scores,
    std::span<const findkey_teddy_suffix_mode> suffix_modes,
    std::span<const int> sigmas) {
    const auto groupings = make_grouping_configurations(strategies, scores);

    std::vector<findkey_teddy_config> configurations;
    configurations.reserve(groupings.size() * suffix_modes.size() *
                           sigmas.size());

    for (const auto grouping : groupings) {
        for (const auto suffix_mode : suffix_modes) {
            for (const int sigma : sigmas) {
                configurations.push_back({grouping, suffix_mode, sigma});
            }
        }
    }

    return configurations;
}

std::vector<findkey_teddy_config> all_teddy_configurations() {
    return make_teddy_configurations(ALL_GROUPING_STRATEGIES,
                                     ALL_GROUPING_SCORES, ALL_SUFFIX_MODES,
                                     ALL_SIGMAS);
}

}  // namespace teddy
