#include "teddy/configurations.h"

#include "teddy/grouping/strategy.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>

TEST(TeddyConfigurationsTest, ExpandsEveryMeaningfulGroupingCombination) {
    const auto configurations = teddy::all_grouping_configurations();

    size_t expected_size = 0;
    for (const auto strategy : teddy::ALL_GROUPING_STRATEGIES) {
        expected_size += teddy::grouping::grouping_strategy_uses_score(strategy)
                             ? teddy::ALL_GROUPING_SCORES.size()
                             : 1;
    }
    ASSERT_EQ(configurations.size(), expected_size);

    for (const auto strategy : teddy::ALL_GROUPING_STRATEGIES) {
        const size_t strategy_count = static_cast<size_t>(
            std::count_if(configurations.begin(), configurations.end(),
                          [strategy](const auto& configuration) {
                              return configuration.strategy == strategy;
                          }));
        const size_t expected_strategy_count =
            teddy::grouping::grouping_strategy_uses_score(strategy)
                ? teddy::ALL_GROUPING_SCORES.size()
                : 1;
        EXPECT_EQ(strategy_count, expected_strategy_count);
    }
}

TEST(TeddyConfigurationsTest, UsesCanonicalScoreForScoreIndependentStrategy) {
    constexpr findkey_teddy_compile_grouping_strategy strategies[] = {
        TEDDY_COMPILE_HASH_STD,
    };

    const auto configurations = teddy::make_grouping_configurations(
        strategies, teddy::ALL_GROUPING_SCORES);

    ASSERT_EQ(configurations.size(), 1u);
    EXPECT_EQ(configurations.front().strategy, TEDDY_COMPILE_HASH_STD);
    EXPECT_EQ(configurations.front().score, TEDDY_GROUPING_SCORE_PAPER);
}

TEST(TeddyConfigurationsTest, ExpandsTheCompleteTeddyConfigurationMatrix) {
    const auto grouping_configurations = teddy::all_grouping_configurations();
    const auto configurations = teddy::all_teddy_configurations();

    EXPECT_EQ(configurations.size(), grouping_configurations.size() *
                                         teddy::ALL_SUFFIX_MODES.size() *
                                         teddy::ALL_SIGMAS.size());
}
