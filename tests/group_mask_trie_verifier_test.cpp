#include "teddy/compile.h"
#include "teddy/verification/verifiers/group_mask_trie.h"
#include "teddy/verify.h"
#include "teddy_verifier_test_utils.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace {

uint8_t group_bit_for_suffix(const teddy::CompilationData& compilation,
                             uint32_t suffix_id) {
    for (size_t group = 0; group < compilation.group_suffix_ids.size();
         ++group) {
        for (const uint32_t grouped_suffix_id :
             compilation.group_suffix_ids[group]) {
            if (grouped_suffix_id == suffix_id) {
                return static_cast<uint8_t>(uint8_t{1} << group);
            }
        }
    }
    return 0;
}

}  // namespace

TEST(TeddyGroupMaskTrieVerifierTest, AcceptsOnlyTheKeysInCandidateGroups) {
    constexpr std::string_view data = R"("alpha":1)";
    const std::vector<std::string_view> keys = {"alpha", "beta"};
    const findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::GroupMaskTrieVerifier verifier(context);

    const uint8_t alpha_group =
        group_bit_for_suffix(compilation, compilation.key_suffix_ids[0]);
    const uint8_t beta_group =
        group_bit_for_suffix(compilation, compilation.key_suffix_ids[1]);
    ASSERT_NE(alpha_group, 0);
    ASSERT_NE(beta_group, 0);
    ASSERT_NE(alpha_group, beta_group);

    const teddy::CandidateResult accepted =
        teddy::verify_json_key_candidate(data, 6, alpha_group, verifier);
    const teddy::CandidateResult rejected =
        teddy::verify_json_key_candidate(data, 6, beta_group, verifier);

    ASSERT_EQ(accepted.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(accepted.position, 1u);
    EXPECT_EQ(accepted.key_id, 0u);
    EXPECT_EQ(rejected.type, teddy::CANDIDATE_KEY_NOT_FOUND);
}

TEST(TeddyGroupMaskTrieVerifierTest,
     TerminalMaskDoesNotBorrowMembershipFromLongerKeys) {
    const std::vector<std::string_view> keys = {"a", "ba"};

    // Keep two otherwise identical suffix records so the test can place the
    // shorter and longer key in different groups. This exercises the case
    // where a node is both a terminal and a prefix of a longer reversed key.
    teddy::SuffixSet suffixes{
        .sigma = 1,
        .end_quote_offset = 1,
        .data = {teddy::Suffix{'a'}, teddy::Suffix{'a'}},
        .key_suffix_ids = {0, 1},
    };
    const teddy::CompilationData compilation =
        teddy::compile(std::move(suffixes), FINDKEY_TEDDY_GROUPING_CONFIG_INIT);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::GroupMaskTrieVerifier verifier(context);

    const uint8_t short_key_group = group_bit_for_suffix(compilation, 0);
    const uint8_t long_key_group = group_bit_for_suffix(compilation, 1);
    ASSERT_NE(short_key_group, 0);
    ASSERT_NE(long_key_group, 0);
    ASSERT_NE(short_key_group, long_key_group);

    constexpr std::string_view data = R"("a":1)";
    const teddy::CandidateResult accepted =
        teddy::verify_json_key_candidate(data, 2, short_key_group, verifier);
    const teddy::CandidateResult rejected =
        teddy::verify_json_key_candidate(data, 2, long_key_group, verifier);

    ASSERT_EQ(accepted.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(accepted.key_id, 0u);
    EXPECT_EQ(rejected.type, teddy::CANDIDATE_KEY_NOT_FOUND);
}

TEST(TeddyGroupMaskTrieVerifierTest, DuplicateKeysReturnTheFirstId) {
    const teddy::CandidateResult result =
        findkey_test::verify_candidate<teddy::GroupMaskTrieVerifier>(
            R"("teddy":1)", 6, {"teddy", "teddy"});

    ASSERT_EQ(result.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(result.position, 1u);
    EXPECT_EQ(result.key_id, 0u);
}
