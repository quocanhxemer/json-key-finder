#include "core/findkey_error.h"
#include "teddy/compile.h"
#include "teddy/verification/metadata.h"
#include "teddy/verification/verifiers/per_suffix_trie.h"
#include "teddy/verify.h"

#include <gtest/gtest.h>

#include <string_view>
#include <utility>
#include <vector>

TEST(TeddyPerSuffixTrieVerifierTest, ReportsMemoryUsage) {
    const std::vector<std::string_view> keys = {"alpha", "omega", "beta"};
    const findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::PerSuffixTrieVerifier verifier(context);

    ASSERT_EQ(compilation.suffixes.size(), 3u);

    const teddy::VerifierCompilationMetadata metadata =
        teddy::get_verifier_compilation_metadata(verifier);
    EXPECT_EQ(metadata.strategy, TEDDY_VERIFY_PER_SUFFIX_TRIE);
    EXPECT_EQ(metadata.verifier_size_bytes, verifier.memory_usage_bytes());
    EXPECT_GT(metadata.verifier_size_bytes, sizeof(verifier));
}

TEST(TeddyPerSuffixTrieVerifierTest, MatchesKeysWithTheSameSuffix) {
    const std::vector<std::string_view> keys = {"alpha", "xalpha"};
    const findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::PerSuffixTrieVerifier verifier(context);

    ASSERT_EQ(compilation.suffixes.size(), 1u);

    const teddy::CandidateResult alpha =
        teddy::verify_json_key_candidate(R"("alpha":1)", 6, 0xFF, verifier);
    const teddy::CandidateResult xalpha =
        teddy::verify_json_key_candidate(R"("xalpha":1)", 7, 0xFF, verifier);

    ASSERT_EQ(alpha.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(alpha.key_id, 0u);
    ASSERT_EQ(xalpha.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(xalpha.key_id, 1u);
}

TEST(TeddyPerSuffixTrieVerifierTest, UsesExactSuffixToSelectTrie) {
    const std::vector<std::string_view> keys = {"alpha", "beta"};
    const findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::PerSuffixTrieVerifier verifier(context);

    const teddy::CandidateResult alpha =
        teddy::verify_json_key_candidate(R"("alpha":1)", 6, 0xFF, verifier);
    const teddy::CandidateResult beta =
        teddy::verify_json_key_candidate(R"("beta":1)", 5, 0xFF, verifier);
    const teddy::CandidateResult unknown_suffix =
        teddy::verify_json_key_candidate(R"("alpza":1)", 6, 0xFF, verifier);

    ASSERT_EQ(alpha.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(alpha.key_id, 0u);
    ASSERT_EQ(beta.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(beta.key_id, 1u);
    EXPECT_EQ(unknown_suffix.type, teddy::CANDIDATE_KEY_NOT_FOUND);
}

TEST(TeddyPerSuffixTrieVerifierTest, SupportsQuotedSuffixes) {
    const std::vector<std::string_view> keys = {"alpha"};
    findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    config.suffix_mode = TEDDY_SUFFIX_QUOTED;
    config.sigma = 4;

    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::PerSuffixTrieVerifier verifier(context);

    ASSERT_EQ(compilation.sigma, 5);
    const teddy::CandidateResult result =
        teddy::verify_json_key_candidate(R"("alpha":1)", 6, 0xFF, verifier);

    ASSERT_EQ(result.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(result.position, 1u);
    EXPECT_EQ(result.key_id, 0u);
}

TEST(TeddyPerSuffixTrieVerifierTest, RejectsCandidatesShorterThanSigma) {
    const std::vector<std::string_view> keys = {"alpha"};
    const findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::PerSuffixTrieVerifier verifier(context);

    const teddy::CandidateResult result =
        teddy::verify_json_key_candidate(R"("":1)", 1, 0xFF, verifier);

    EXPECT_EQ(result.type, teddy::CANDIDATE_KEY_NOT_FOUND);
}

TEST(TeddyPerSuffixTrieVerifierTest, RejectsDuplicateEncodedSuffixes) {
    const std::vector<std::string_view> keys = {"a", "ba"};
    teddy::SuffixSet suffixes{
        .sigma = 1,
        .end_quote_offset = 1,
        .data = {teddy::Suffix{'a'}, teddy::Suffix{'a'}},
        .key_suffix_ids = {0, 1},
    };
    const teddy::CompilationData compilation =
        teddy::compile(std::move(suffixes), FINDKEY_TEDDY_GROUPING_CONFIG_INIT);
    const teddy::VerificationBuildContext context{keys, compilation};

    try {
        const teddy::PerSuffixTrieVerifier verifier(context);
        (void)verifier;
        FAIL() << "Expected duplicate suffixes to be rejected";
    } catch (const FindkeyError& error) {
        EXPECT_EQ(error.code(), FindkeyErrorCode::INVALID_ARGUMENT);
    }
}

TEST(TeddyPerSuffixTrieVerifierTest, PreservesMissingOpeningQuote) {
    const std::vector<std::string_view> keys = {"abc"};
    const findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::PerSuffixTrieVerifier verifier(context);

    const teddy::CandidateResult result =
        teddy::verify_json_key_candidate(R"(abc":1)", 3, 0xFF, verifier);

    EXPECT_EQ(result.type, teddy::CANDIDATE_MISSING_OPEN_QUOTE);
}
