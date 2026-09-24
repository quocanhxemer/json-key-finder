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

TEST(TeddyPerSuffixTrieVerifierTest, StoresOnlyTheUnmatchedKeyPrefixes) {
    const std::vector<std::string_view> keys = {"abc", "def", "ghij"};
    findkey_teddy_config short_suffix_config = FINDKEY_TEDDY_CONFIG_INIT;
    short_suffix_config.sigma = 1;
    const teddy::CompilationData short_suffix_compilation =
        teddy::compile(keys, short_suffix_config);
    const teddy::VerificationBuildContext short_suffix_context{
        keys, short_suffix_compilation};
    const teddy::PerSuffixTrieVerifier short_suffix_verifier(
        short_suffix_context);

    findkey_teddy_config long_suffix_config = FINDKEY_TEDDY_CONFIG_INIT;
    long_suffix_config.sigma = 3;
    const teddy::CompilationData long_suffix_compilation =
        teddy::compile(keys, long_suffix_config);
    const teddy::VerificationBuildContext long_suffix_context{
        keys, long_suffix_compilation};
    const teddy::PerSuffixTrieVerifier long_suffix_verifier(
        long_suffix_context);

    ASSERT_EQ(short_suffix_compilation.suffixes.size(),
              long_suffix_compilation.suffixes.size());
    EXPECT_LT(long_suffix_verifier.memory_usage_bytes(),
              short_suffix_verifier.memory_usage_bytes());
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

TEST(TeddyPerSuffixTrieVerifierTest, MatchesAfterRemovingTheEntireKeySuffix) {
    const std::vector<std::string_view> keys = {"abc", "xabc"};
    const findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::PerSuffixTrieVerifier verifier(context);

    ASSERT_EQ(compilation.sigma, 3);
    ASSERT_EQ(compilation.suffixes.size(), 1u);

    const teddy::CandidateResult empty_prefix =
        teddy::verify_json_key_candidate(R"("abc":1)", 4, 0xFF, verifier);
    const teddy::CandidateResult nonempty_prefix =
        teddy::verify_json_key_candidate(R"("xabc":1)", 5, 0xFF, verifier);

    ASSERT_EQ(empty_prefix.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(empty_prefix.position, 1u);
    EXPECT_EQ(empty_prefix.key_id, 0u);
    ASSERT_EQ(nonempty_prefix.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(nonempty_prefix.position, 1u);
    EXPECT_EQ(nonempty_prefix.key_id, 1u);
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

TEST(TeddyPerSuffixTrieVerifierTest,
     MatchesAfterRemovingTheEntireQuotedKeySuffix) {
    const std::vector<std::string_view> keys = {"abcd"};
    findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    config.suffix_mode = TEDDY_SUFFIX_QUOTED;
    config.sigma = 4;

    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::PerSuffixTrieVerifier verifier(context);

    ASSERT_EQ(compilation.sigma, 5);
    const teddy::CandidateResult result =
        teddy::verify_json_key_candidate(R"("abcd":1)", 5, 0xFF, verifier);

    ASSERT_EQ(result.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(result.position, 1u);
    EXPECT_EQ(result.key_id, 0u);
}

TEST(TeddyPerSuffixTrieVerifierTest,
     AcceptsAnEscapedQuoteAtThePrefixSuffixBoundary) {
    const std::vector<std::string_view> keys = {R"(a\"bc)"};
    const findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::PerSuffixTrieVerifier verifier(context);

    const teddy::CandidateResult result =
        teddy::verify_json_key_candidate(R"("a\"bc":1)", 6, 0xFF, verifier);

    ASSERT_EQ(result.type, teddy::CANDIDATE_MATCH);
    EXPECT_EQ(result.position, 1u);
    EXPECT_EQ(result.key_id, 0u);
}

TEST(TeddyPerSuffixTrieVerifierTest,
     RejectsAnUnescapedQuoteInTheMatchedSuffix) {
    const std::vector<std::string_view> keys = {R"(a"bc)"};
    const findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};
    const teddy::PerSuffixTrieVerifier verifier(context);

    const teddy::CandidateResult result =
        teddy::verify_json_key_candidate(R"("a"bc":1)", 5, 0xFF, verifier);

    EXPECT_EQ(result.type, teddy::CANDIDATE_KEY_NOT_FOUND);
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
