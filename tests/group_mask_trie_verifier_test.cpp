#include "teddy/compile.h"
#include "teddy/verification/verifiers/group_mask_trie.h"
#include "teddy/verify.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace {

uint8_t group_bit_for_suffix(const teddy::CompilationData& compilation,
                             uint32_t suffix_id) {
    for (size_t group = 0; group < compilation.num_groups(); ++group) {
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
