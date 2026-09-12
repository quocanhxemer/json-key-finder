#include "utils.h"

#include "matchers/matcher_teddy_baseline.h"
#include "teddy/compile.h"
#include "teddy/suffix.h"
#include "teddy/verification/verifiers/plain_trie.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace {

constexpr int TEST_SIGMA = 3;

teddy::CompilationData compile_ddy_group() {
    const std::vector<std::string_view> keys = {"teddy"};
    return teddy::compile(
        keys, findkey_test::make_teddy_config(TEDDY_SUFFIX_RAW, TEST_SIGMA,
                                              TEDDY_VERIFY_TRIE));
}

void expect_group_bit_cleared(const teddy::CompilationData& compilation,
                              const std::size_t group,
                              const int position,
                              const uint8_t byte) {
    const uint8_t group_bit = static_cast<uint8_t>(1u << group);
    const uint8_t low_nibble = byte & 0x0F;
    const uint8_t high_nibble = byte >> 4;

    SCOPED_TRACE(::testing::Message()
                 << "group: " << group << ", position: " << position
                 << ", byte: " << static_cast<unsigned int>(byte));
    EXPECT_EQ(compilation.low_table[position][low_nibble] & group_bit, 0u);
    EXPECT_EQ(compilation.high_table[position][high_nibble] & group_bit, 0u);
}

bool group_prefilter_accepts(const teddy::CompilationData& compilation,
                             const std::size_t group,
                             const teddy::Suffix& suffix) {
    const uint8_t group_bit = static_cast<uint8_t>(1u << group);
    for (int position = 0; position < compilation.sigma; ++position) {
        const uint8_t byte = suffix[position];
        const uint8_t transition =
            compilation.low_table[position][byte & 0x0F] |
            compilation.high_table[position][byte >> 4];
        if ((transition & group_bit) != 0) {
            return false;
        }
    }
    return true;
}

}  // namespace

TEST(TeddyTransitionTableTest, ClearsTheGroupBitForEveryDdyNibble) {
    const teddy::CompilationData compilation = compile_ddy_group();
    constexpr teddy::Suffix expected_suffix{'d', 'd', 'y'};

    ASSERT_EQ(compilation.sigma, TEST_SIGMA);
    ASSERT_EQ(compilation.num_groups, 1);
    ASSERT_EQ(compilation.suffixes.size(), 1u);
    ASSERT_EQ(compilation.group_suffix_ids.size(), 1u);
    ASSERT_EQ(compilation.group_suffix_ids.front(), std::vector<uint32_t>{0});
    ASSERT_EQ(compilation.suffixes.front(), expected_suffix);

    expect_group_bit_cleared(compilation, 0, 0, 'd');
    expect_group_bit_cleared(compilation, 0, 1, 'd');
    expect_group_bit_cleared(compilation, 0, 2, 'y');
}

TEST(TeddyTransitionTableTest, RejectsAnUnrelatedSuffix) {
    const teddy::CompilationData compilation = compile_ddy_group();
    constexpr teddy::Suffix compiled_suffix{'d', 'd', 'y'};
    constexpr teddy::Suffix unrelated_suffix{'a', 'b', 'c'};

    EXPECT_TRUE(group_prefilter_accepts(compilation, 0, compiled_suffix));
    EXPECT_FALSE(group_prefilter_accepts(compilation, 0, unrelated_suffix));
}

TEST(TeddyTransitionTableTest, CrossProductHitIsRejectedByExactVerification) {
    const std::vector<std::string_view> keys = {
        "AAA", "AAR", "ccc", "ttt", "%%%", "666", "GGG", "XXX", "iii",
    };
    findkey_teddy_config config = findkey_test::make_teddy_config(
        TEDDY_SUFFIX_RAW, TEST_SIGMA, TEDDY_VERIFY_TRIE);
    config.grouping.strategy = TEDDY_COMPILE_GREEDY_MIN_DELTA;
    config.grouping.score = TEDDY_GROUPING_SCORE_NIBBLE_COUNT;

    const teddy::CompilationData compilation = teddy::compile(keys, config);
    std::size_t cross_product_group = compilation.group_suffix_ids.size();
    for (std::size_t group = 0; group < compilation.group_suffix_ids.size();
         ++group) {
        const auto& suffix_ids = compilation.group_suffix_ids[group];
        bool contains_aaa = false;
        bool contains_aar = false;
        for (const uint32_t suffix_id : suffix_ids) {
            contains_aaa |= suffix_id == 0;
            contains_aar |= suffix_id == 1;
        }
        if (contains_aaa && contains_aar) {
            cross_product_group = group;
            break;
        }
    }

    ASSERT_LT(cross_product_group, compilation.group_suffix_ids.size());
    ASSERT_EQ(compilation.group_suffix_ids[cross_product_group].size(), 2u);

    // At the final position, A contributes high nibble 4 and R contributes
    // low nibble 2. Their cross-product admits B (0x42), although AAB is not
    // one of the compiled suffixes.
    constexpr teddy::Suffix cross_product_suffix{'A', 'A', 'B'};
    ASSERT_TRUE(group_prefilter_accepts(compilation, cross_product_group,
                                        cross_product_suffix));

    constexpr std::string_view json = R"({"AAB":1})";
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::PlainTrieVerifier verifier(context);
    findkey_teddy_stats stats{};
    const std::vector<findkey_result> results =
        matcher_teddy_baseline(json, compilation, verifier, &stats);

    EXPECT_TRUE(results.empty());
    EXPECT_EQ(stats.prefilter_hit_lanes, 1u);
    EXPECT_EQ(stats.prefilter_hit_groups, 1u);
    EXPECT_EQ(stats.fp_type1_lanes, 1u);
    EXPECT_EQ(stats.fp_type1_groups, 1u);
    EXPECT_EQ(stats.fp_type2_lanes, 0u);
    EXPECT_EQ(stats.reject_key_not_found, 1u);
    EXPECT_EQ(stats.exact_matches, 0u);

    const findkey_test::ApiRun scalar =
        findkey_test::run_findkey(json, keys, SCALAR);
    ASSERT_TRUE(findkey_test::expect_success(scalar));
    ASSERT_EQ(scalar.total, 0u);
    findkey_test::expect_teddy_matchers_match(scalar, json, keys, &config);
}
