#include "teddy_verifier_test_utils.h"

#include "teddy/verification/verifiers/plain_trie.h"

#include <gtest/gtest.h>

#include <string_view>
#include <vector>

TEST(TeddyPlainTrieVerifierTest, ReturnsTheCorrectKey) {
    constexpr std::string_view data = R"("teddy":1)";
    const std::vector<std::string_view> keys = {"teddy", "other"};

    const teddy::CandidateResult result =
        findkey_test::verify_candidate<teddy::PlainTrieVerifier>(data, 6, keys);

    EXPECT_EQ(result.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(result.position, 1u);
    EXPECT_EQ(result.key_id, 0u);
}

TEST(TeddyPlainTrieVerifierTest, RejectsAnUnknownCompleteKeyWithAKnownSuffix) {
    constexpr std::string_view data = R"("daddy":1)";
    const std::vector<std::string_view> keys = {"teddy"};

    const teddy::CandidateResult result =
        findkey_test::verify_candidate<teddy::PlainTrieVerifier>(data, 6, keys);

    EXPECT_EQ(result.type, teddy::CANDIDATE_KEY_NOT_FOUND);
}

TEST(TeddyPlainTrieVerifierTest, RejectsAKeyLongerThanTheLongestTriePath) {
    const teddy::CandidateResult result =
        findkey_test::verify_candidate<teddy::PlainTrieVerifier>(
            R"("xteddy":1)", 7, {"teddy"});

    EXPECT_EQ(result.type, teddy::CANDIDATE_KEY_NOT_FOUND);
}

TEST(TeddyPlainTrieVerifierTest, RecognizesAKeyThatIsASuffixOfAnother) {
    const std::vector<std::string_view> keys = {"id", "user_id"};

    const teddy::CandidateResult short_key =
        findkey_test::verify_candidate<teddy::PlainTrieVerifier>(R"("id":1)", 3,
                                                                 keys);
    const teddy::CandidateResult long_key =
        findkey_test::verify_candidate<teddy::PlainTrieVerifier>(
            R"("user_id":1)", 8, keys);

    ASSERT_EQ(short_key.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(short_key.position, 1u);
    EXPECT_EQ(short_key.key_id, 0u);
    ASSERT_EQ(long_key.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(long_key.position, 1u);
    EXPECT_EQ(long_key.key_id, 1u);
}
