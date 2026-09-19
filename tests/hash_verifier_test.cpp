#include "teddy_verifier_test_utils.h"

#include "teddy/verification/verifiers/hash.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

TEST(TeddyHashVerifierTest, ChecksKnownKeyRangeDirectly) {
    const std::vector<std::string_view> keys = {"teddy", "other"};
    const teddy::HashVerifier verifier(keys);

    const teddy::CandidateResult known = verifier.check_key("teddy", 17);
    ASSERT_EQ(known.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(known.position, 17u);
    EXPECT_EQ(known.key_id, 0u);

    const teddy::CandidateResult unknown = verifier.check_key("daddy", 17);
    EXPECT_EQ(unknown.type, teddy::CANDIDATE_KEY_NOT_FOUND);
}

TEST(TeddyHashVerifierTest, FindsKnownAndUnknownKeys) {
    const std::vector<std::string_view> keys = {"teddy", "other"};

    const teddy::CandidateResult known =
        findkey_test::verify_candidate<teddy::HashVerifier>(R"("teddy":1)", 6,
                                                            keys);
    EXPECT_EQ(known.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(known.position, 1u);
    EXPECT_EQ(known.key_id, 0u);

    const teddy::CandidateResult unknown =
        findkey_test::verify_candidate<teddy::HashVerifier>(R"("daddy":1)", 6,
                                                            keys);
    EXPECT_EQ(unknown.type, teddy::CANDIDATE_KEY_NOT_FOUND);
}

TEST(TeddyHashVerifierTest, DuplicateKeysReturnTheFirstId) {
    const teddy::CandidateResult result =
        findkey_test::verify_candidate<teddy::HashVerifier>(
            R"("teddy":1)", 6, {"teddy", "other", "teddy"});

    ASSERT_EQ(result.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(result.position, 1u);
    EXPECT_EQ(result.key_id, 0u);
}

TEST(TeddyHashVerifierTest, RecognizesKeysThatSuffixOtherKeys) {
    const teddy::CandidateResult short_key =
        findkey_test::verify_candidate<teddy::HashVerifier>(R"("id":1)", 3,
                                                            {"id", "user_id"});
    const teddy::CandidateResult long_key =
        findkey_test::verify_candidate<teddy::HashVerifier>(R"("user_id":1)", 8,
                                                            {"id", "user_id"});

    ASSERT_EQ(short_key.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(short_key.position, 1u);
    EXPECT_EQ(short_key.key_id, 0u);
    ASSERT_EQ(long_key.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(long_key.position, 1u);
    EXPECT_EQ(long_key.key_id, 1u);
}

TEST(TeddyHashVerifierTest, HandlesEscapedQuotesAndBackslashes) {
    constexpr std::string_view data = R"("escaped\"key":1,"backslash\\key":2)";
    const std::vector<std::string_view> keys = {R"(escaped\"key)",
                                                R"(backslash\\key)"};

    const teddy::CandidateResult escaped_quote =
        findkey_test::verify_candidate<teddy::HashVerifier>(data, 13, keys);
    const teddy::CandidateResult escaped_backslash =
        findkey_test::verify_candidate<teddy::HashVerifier>(data, 32, keys);

    ASSERT_EQ(escaped_quote.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(escaped_quote.position, 1u);
    EXPECT_EQ(escaped_quote.key_id, 0u);
    ASSERT_EQ(escaped_backslash.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(escaped_backslash.key_id, 1u);
}

TEST(TeddyHashVerifierTest, ReportsEmptyAndMissingOpeningQuotes) {
    const std::vector<std::string_view> keys = {"teddy"};

    const teddy::CandidateResult empty =
        findkey_test::verify_candidate<teddy::HashVerifier>(R"("":1)", 1, keys);
    EXPECT_EQ(empty.type, teddy::CANDIDATE_KEY_NOT_FOUND);

    const teddy::CandidateResult missing =
        findkey_test::verify_candidate<teddy::HashVerifier>(R"(teddy":1)", 5,
                                                            keys);
    EXPECT_EQ(missing.type, teddy::CANDIDATE_MISSING_OPEN_QUOTE);
}

TEST(TeddyHashVerifierTest, BoundsOpeningQuoteSearchByMaximumKeyLength) {
    const teddy::CandidateResult result =
        findkey_test::verify_candidate<teddy::HashVerifier>(R"("oversized":1)",
                                                            10, {"id"});

    EXPECT_EQ(result.type, teddy::CANDIDATE_MISSING_OPEN_QUOTE);
}

TEST(TeddyHashVerifierTest, PreservesNonAsciiBytes) {
    const std::string key = "caf\xC3\xA9";
    std::string data = "\"";
    data += key;
    data += "\":1";

    const teddy::CandidateResult result =
        findkey_test::verify_candidate<teddy::HashVerifier>(
            data, 1 + key.size(), {key});

    ASSERT_EQ(result.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(result.position, 1u);
    EXPECT_EQ(result.key_id, 0u);
}
