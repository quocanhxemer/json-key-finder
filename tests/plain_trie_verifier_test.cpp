#include "teddy_verifier_test_utils.h"

#include "teddy/verification/verifiers/plain_trie.h"

#include <gtest/gtest.h>

#include <cstddef>
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

TEST(TeddyPlainTrieVerifierTest, ReportsMalformedJsonContext) {
    struct TestCase {
        std::string_view name;
        std::string_view data;
        std::size_t end_quote;
        std::vector<std::string_view> keys;
        teddy::CandidateType expected_type;
    };

    const std::vector<TestCase> cases = {
        {"missing closing quote",
         R"("teddy:1)",
         6,
         {"teddy"},
         teddy::CANDIDATE_BAD_END_QUOTE},
        {"escaped closing quote",
         R"("abc\":1)",
         5,
         {"abc\\"},
         teddy::CANDIDATE_INVALID_QUOTE},
        {"missing colon",
         R"("teddy" 1)",
         6,
         {"teddy"},
         teddy::CANDIDATE_MISSING_COLON},
        {"missing opening quote",
         R"(teddy":1)",
         5,
         {"teddy"},
         teddy::CANDIDATE_MISSING_OPEN_QUOTE},
    };

    for (const auto& test_case : cases) {
        SCOPED_TRACE(::testing::Message() << "case: " << test_case.name);
        const teddy::CandidateResult result =
            findkey_test::verify_candidate<teddy::PlainTrieVerifier>(
                test_case.data, test_case.end_quote, test_case.keys);
        EXPECT_EQ(result.type, test_case.expected_type);
    }
}

TEST(TeddyPlainTrieVerifierTest, AllowsWhitespaceBeforeTheColon) {
    constexpr std::string_view data = "\"teddy\" \n\t:1";
    const std::vector<std::string_view> keys = {"teddy"};

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

TEST(TeddyPlainTrieVerifierTest, DuplicateKeysReturnTheFirstId) {
    constexpr std::string_view data = R"("teddy":1)";
    const std::vector<std::string_view> keys = {"teddy", "teddy"};

    const teddy::CandidateResult result =
        findkey_test::verify_candidate<teddy::PlainTrieVerifier>(data, 6, keys);

    ASSERT_EQ(result.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(result.position, 1u);
    EXPECT_EQ(result.key_id, 0u);
}
