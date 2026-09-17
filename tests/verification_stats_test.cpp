#include "utils.h"

#include "findkey.h"
#include "matchers/matcher_teddy_baseline.h"
#include "teddy/compile.h"
#include "teddy/verification/verifiers/plain_trie.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string_view>
#include <vector>

namespace {

struct BaselineRun {
    std::vector<findkey_result> results;
    findkey_teddy_stats stats{};
};

BaselineRun run_baseline_with_stats(const std::string_view data,
                                    const std::vector<std::string_view>& keys) {
    const findkey_teddy_config config = findkey_test::make_teddy_config(
        TEDDY_SUFFIX_RAW, 3, TEDDY_VERIFY_PLAIN_TRIE);
    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::PlainTrieVerifier verifier(context);

    BaselineRun run;
    run.results =
        matcher_teddy_baseline(data, compilation, verifier, &run.stats);
    return run;
}

uint64_t total_rejections(const findkey_teddy_stats& stats) {
    return stats.reject_bad_end_quote + stats.reject_invalid_quote +
           stats.reject_missing_colon + stats.reject_missing_open_quote +
           stats.reject_key_not_found;
}

}  // namespace

TEST(TeddyStatisticsTest, CountsAnExactMatch) {
    constexpr std::string_view data = R"({"teddy":1})";
    const std::vector<std::string_view> keys = {"teddy"};

    const BaselineRun run = run_baseline_with_stats(data, keys);

    ASSERT_EQ(run.results.size(), 1u);
    EXPECT_EQ(run.results.front().position, 2u);
    EXPECT_EQ(run.results.front().key_id, 0u);
    EXPECT_EQ(run.stats.prefilter_hit_lanes, 1u);
    EXPECT_EQ(run.stats.prefilter_hit_groups, 1u);
    EXPECT_EQ(run.stats.exact_matches, 1u);
    EXPECT_EQ(run.stats.fp_type1_lanes, 0u);
    EXPECT_EQ(run.stats.fp_type1_groups, 0u);
    EXPECT_EQ(run.stats.fp_type2_lanes, 0u);
    EXPECT_EQ(total_rejections(run.stats), 0u);
}

TEST(TeddyStatisticsTest, CountsEveryVerifierRejectionForExactSuffixHits) {
    using RejectionCounter = uint64_t findkey_teddy_stats::*;
    struct TestCase {
        std::string_view name;
        std::string_view data;
        std::vector<std::string_view> keys;
        RejectionCounter expected_counter;
    };

    const std::vector<TestCase> cases = {
        {"bad end quote",
         R"({"teddy:1})",
         {"teddy"},
         &findkey_teddy_stats::reject_bad_end_quote},
        {"invalid quote",
         R"({"abc\":1})",
         {"abc\\"},
         &findkey_teddy_stats::reject_invalid_quote},
        {"missing colon",
         R"({"teddy" 1})",
         {"teddy"},
         &findkey_teddy_stats::reject_missing_colon},
        {"missing opening quote",
         R"(teddy":1)",
         {"teddy"},
         &findkey_teddy_stats::reject_missing_open_quote},
        {"key not found",
         R"({"daddy":1})",
         {"teddy"},
         &findkey_teddy_stats::reject_key_not_found},
    };

    for (const auto& test_case : cases) {
        SCOPED_TRACE(::testing::Message() << "case: " << test_case.name);
        const BaselineRun run =
            run_baseline_with_stats(test_case.data, test_case.keys);

        EXPECT_TRUE(run.results.empty());
        EXPECT_EQ(run.stats.prefilter_hit_lanes, 1u);
        EXPECT_EQ(run.stats.prefilter_hit_groups, 1u);
        EXPECT_EQ(run.stats.fp_type1_lanes, 0u);
        EXPECT_EQ(run.stats.fp_type1_groups, 0u);
        EXPECT_EQ(run.stats.fp_type2_lanes, 1u);
        EXPECT_EQ(run.stats.exact_matches, 0u);
        EXPECT_EQ(run.stats.*test_case.expected_counter, 1u);
        EXPECT_EQ(total_rejections(run.stats), 1u);
    }
}
