#include "utils.h"

#include "teddy/compile.h"
#include "teddy/configurations.h"
#include "teddy/grouping.h"
#include "teddy/suffix.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string_view>
#include <vector>

namespace {

using GroupedSuffixIds = std::vector<std::vector<uint32_t>>;

std::vector<teddy::Suffix> make_unique_suffixes(const std::size_t count) {
    std::vector<teddy::Suffix> suffixes;
    suffixes.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        suffixes.push_back({
            static_cast<uint8_t>(0x20 + i),
            static_cast<uint8_t>(0x40 + (i * 3) % 31),
            static_cast<uint8_t>(0x60 + (i * 5) % 31),
            static_cast<uint8_t>(0x80 + (i * 7) % 31),
            static_cast<uint8_t>(0xA0 + (i * 11) % 31),
        });
    }

    return suffixes;
}

void expect_valid_grouping(const GroupedSuffixIds& groups,
                           const std::size_t suffix_count) {
    ASSERT_GT(suffix_count, 0u);
    ASSERT_FALSE(groups.empty());
    EXPECT_LE(groups.size(), static_cast<std::size_t>(teddy::MAX_GROUPS));

    std::vector<std::size_t> freq(suffix_count, 0);
    for (std::size_t i = 0; i < groups.size(); ++i) {
        SCOPED_TRACE(::testing::Message() << "group index: " << i);
        const auto& group = groups[i];
        EXPECT_FALSE(group.empty());

        for (const uint32_t suffix_id : group) {
            ASSERT_LT(suffix_id, suffix_count);
            ++freq[suffix_id];
        }
    }

    for (std::size_t suffix_id = 0; suffix_id < freq.size(); ++suffix_id) {
        SCOPED_TRACE(::testing::Message() << "suffix ID: " << suffix_id);
        EXPECT_EQ(freq[suffix_id], 1u);
    }
}

void expect_valid_routing(const teddy::CompilationData& compilation,
                          const std::size_t key_count) {
    ASSERT_EQ(compilation.key_suffix_ids.size(), key_count);

    for (std::size_t key_id = 0; key_id < key_count; ++key_id) {
        SCOPED_TRACE(::testing::Message() << "key ID: " << key_id);
        const uint32_t suffix_id = compilation.key_suffix_ids[key_id];
        ASSERT_LT(suffix_id, compilation.suffixes.size());
    }
}

bool active_suffixes_are_unique(const std::vector<teddy::Suffix>& suffixes,
                                const int sigma) {
    for (std::size_t i = 0; i < suffixes.size(); ++i) {
        for (std::size_t j = i + 1; j < suffixes.size(); ++j) {
            if (std::equal(suffixes[i].begin(), suffixes[i].begin() + sigma,
                           suffixes[j].begin())) {
                return false;
            }
        }
    }
    return true;
}

uint64_t independently_calculate_nibble_count_score(
    const std::vector<teddy::Suffix>& suffixes,
    const std::vector<uint32_t>& suffix_ids,
    const std::size_t begin,
    const std::size_t end,
    const int sigma) {
    std::array<std::array<bool, 16>, FINDKEY_TEDDY_MAX_SIGMA> low_seen{};
    std::array<std::array<bool, 16>, FINDKEY_TEDDY_MAX_SIGMA> high_seen{};

    for (std::size_t i = begin; i < end; ++i) {
        const teddy::Suffix& suffix = suffixes[suffix_ids[i]];
        for (int byte_index = 0; byte_index < sigma; ++byte_index) {
            low_seen[byte_index][suffix[byte_index] & 0x0F] = true;
            high_seen[byte_index][suffix[byte_index] >> 4] = true;
        }
    }

    uint64_t score = 1;
    for (int byte_index = 0; byte_index < sigma; ++byte_index) {
        score *= static_cast<uint64_t>(std::count(
            low_seen[byte_index].begin(), low_seen[byte_index].end(), true));
        score *= static_cast<uint64_t>(std::count(
            high_seen[byte_index].begin(), high_seen[byte_index].end(), true));
    }
    return score;
}

void collect_contiguous_partition_scores(
    const std::vector<teddy::Suffix>& suffixes,
    const std::vector<uint32_t>& sorted_suffix_ids,
    const std::size_t begin,
    const std::size_t groups_remaining,
    const uint64_t score_so_far,
    const int sigma,
    std::vector<uint64_t>& partition_scores) {
    if (groups_remaining == 1) {
        partition_scores.push_back(score_so_far +
                                   independently_calculate_nibble_count_score(
                                       suffixes, sorted_suffix_ids, begin,
                                       sorted_suffix_ids.size(), sigma));
        return;
    }

    const std::size_t last_end =
        sorted_suffix_ids.size() - (groups_remaining - 1);
    for (std::size_t end = begin + 1; end <= last_end; ++end) {
        const uint64_t group_score = independently_calculate_nibble_count_score(
            suffixes, sorted_suffix_ids, begin, end, sigma);
        collect_contiguous_partition_scores(
            suffixes, sorted_suffix_ids, end, groups_remaining - 1,
            score_so_far + group_score, sigma, partition_scores);
    }
}

uint64_t independently_calculate_total_nibble_count_score(
    const std::vector<teddy::Suffix>& suffixes,
    const GroupedSuffixIds& groups,
    const int sigma) {
    uint64_t total = 0;
    for (const auto& group : groups) {
        total += independently_calculate_nibble_count_score(
            suffixes, group, 0, group.size(), sigma);
    }
    return total;
}

}  // namespace

TEST(TeddyGroupingInvariantsTest,
     EveryStrategyProducesAValidDeterministicGrouping) {
    const std::vector<teddy::Suffix> suffixes = make_unique_suffixes(17);

    for (const auto config : teddy::all_grouping_configurations()) {
        for (int sigma = 1; sigma <= FINDKEY_TEDDY_MAX_SIGMA; ++sigma) {
            SCOPED_TRACE(::testing::Message()
                         << "strategy: " << static_cast<int>(config.strategy)
                         << ", score: " << static_cast<int>(config.score)
                         << ", sigma: " << sigma);
            ASSERT_TRUE(active_suffixes_are_unique(suffixes, sigma));

            // Check determinism
            const GroupedSuffixIds first =
                teddy::build_groups(suffixes, config, sigma);
            const GroupedSuffixIds second =
                teddy::build_groups(suffixes, config, sigma);

            expect_valid_grouping(first, suffixes.size());
            EXPECT_EQ(second, first);
        }
    }
}

TEST(TeddyGroupingInvariantsTest,
     NonHashStrategiesKeepFewerThanEightSuffixesSeparate) {
    const std::set<findkey_teddy_compile_grouping_strategy> hash_strategies = {
        TEDDY_COMPILE_HASH_STD,   TEDDY_COMPILE_HASH_ADLER32,
        TEDDY_COMPILE_HASH_CRC32, TEDDY_COMPILE_HASH_XXHASH,
        TEDDY_COMPILE_HASH_FNV1A,
    };
    const std::vector<teddy::Suffix> suffix_pool = make_unique_suffixes(7);

    for (std::size_t suffix_count = 1; suffix_count < teddy::MAX_GROUPS;
         ++suffix_count) {
        const std::vector<teddy::Suffix> suffixes(
            suffix_pool.begin(), suffix_pool.begin() + suffix_count);
        for (const auto config : teddy::all_grouping_configurations()) {
            // Hash collisions may combine suffixes even when fewer than eight
            // suffixes are provided.
            if (hash_strategies.contains(config.strategy)) {
                continue;
            }

            SCOPED_TRACE(::testing::Message()
                         << "suffix count: " << suffix_count
                         << ", strategy: " << static_cast<int>(config.strategy)
                         << ", score: " << static_cast<int>(config.score));
            const GroupedSuffixIds groups =
                teddy::build_groups(suffixes, config, 3);

            expect_valid_grouping(groups, suffix_count);
            ASSERT_EQ(groups.size(), suffix_count);
            for (const auto& group : groups) {
                EXPECT_EQ(group.size(), 1u);
            }
        }
    }
}

TEST(TeddyGroupingInvariantsTest,
     CompilationRoutesEveryKeyThroughItsDeduplicatedSuffix) {
    const std::vector<std::string_view> keys = {
        "key000", "key001", "key002", "key003",   "key004", "key005",
        "key006", "key007", "key008", "other000", "key003",
    };
    const std::vector<uint32_t> expected_key_suffix_ids = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 3,
    };

    for (const auto grouping : teddy::all_grouping_configurations()) {
        for (const auto suffix_mode : teddy::ALL_SUFFIX_MODES) {
            SCOPED_TRACE(::testing::Message()
                         << "strategy: " << static_cast<int>(grouping.strategy)
                         << ", score: " << static_cast<int>(grouping.score)
                         << ", suffix mode: " << static_cast<int>(suffix_mode));
            findkey_teddy_config config = findkey_test::make_teddy_config(
                suffix_mode, 3, TEDDY_VERIFY_PLAIN_TRIE);
            config.grouping = grouping;

            const teddy::CompilationData compilation =
                teddy::compile(keys, config);

            ASSERT_EQ(compilation.suffixes.size(), 9u);
            EXPECT_EQ(compilation.key_suffix_ids, expected_key_suffix_ids);
            expect_valid_grouping(compilation.group_suffix_ids,
                                  compilation.suffixes.size());
            expect_valid_routing(compilation, keys.size());

            // 9 suffixes must occupy at most 8 groups
            // test pigeonhole principle
            EXPECT_TRUE(std::any_of(
                compilation.group_suffix_ids.begin(),
                compilation.group_suffix_ids.end(),
                [](const auto& group) { return group.size() > 1; }));
        }
    }
}

TEST(TeddySortedOptimalPartitionTest,
     HasNoHigherScoreThanAnyContiguousPartition) {
    constexpr int sigma = 3;
    const std::vector<teddy::Suffix> suffixes = {
        {'E', 'E', 'E'}, {'A', 'A', 'A'}, {'O', 'O', 'O'}, {'C', 'C', 'D'},
        {'K', 'K', 'K'}, {'A', 'A', 'B'}, {'M', 'M', 'M'}, {'C', 'C', 'C'},
        {'I', 'I', 'I'}, {'G', 'G', 'G'},
    };
    constexpr findkey_teddy_grouping_config config = {
        TEDDY_COMPILE_SORTED_SUFFIX_OPTIMAL_PARTITION,
        TEDDY_GROUPING_SCORE_NIBBLE_COUNT,
    };

    const GroupedSuffixIds actual =
        teddy::build_groups(suffixes, config, sigma);
    expect_valid_grouping(actual, suffixes.size());
    ASSERT_EQ(actual.size(), static_cast<std::size_t>(teddy::MAX_GROUPS));

    std::vector<uint32_t> flattened_actual;
    for (const auto& group : actual) {
        flattened_actual.insert(flattened_actual.end(), group.begin(),
                                group.end());
    }

    const auto active_suffix_less = [&suffixes](const uint32_t left_id,
                                                const uint32_t right_id) {
        return std::lexicographical_compare(
            suffixes[left_id].begin(), suffixes[left_id].begin() + sigma,
            suffixes[right_id].begin(), suffixes[right_id].begin() + sigma);
    };
    ASSERT_TRUE(std::is_sorted(flattened_actual.begin(), flattened_actual.end(),
                               active_suffix_less));

    std::vector<uint64_t> partition_scores;
    collect_contiguous_partition_scores(suffixes, flattened_actual, 0,
                                        teddy::MAX_GROUPS, 0, sigma,
                                        partition_scores);

    ASSERT_EQ(partition_scores.size(), 36u);
    const auto [minimum, maximum] =
        std::minmax_element(partition_scores.begin(), partition_scores.end());
    ASSERT_NE(minimum, partition_scores.end());
    ASSERT_NE(maximum, partition_scores.end());
    EXPECT_LT(*minimum, *maximum);
    EXPECT_EQ(*minimum, 10u);
    EXPECT_EQ(independently_calculate_total_nibble_count_score(suffixes, actual,
                                                               sigma),
              *minimum);
}
