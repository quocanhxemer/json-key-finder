#include "utils.h"

#include "teddy/configurations.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace {

using findkey_test::ApiRun;
using findkey_test::expect_success;
using findkey_test::expect_teddy_matchers_match;
using findkey_test::expect_teddy_matches_scalar;
using findkey_test::load_json_fixture;
using findkey_test::run_findkey;

}  // namespace

TEST(FindkeyPublicApiTest, ScalarReturnsExpectedOrderedResults) {
    constexpr std::string_view json =
        R"({"name":"Anh","id":42,"ignored":true})";
    const std::vector<std::string_view> keys = {"name", "id", "random"};

    const ApiRun run = run_findkey(json, keys, SCALAR);

    ASSERT_EQ(run.status, FINDKEY_OK);
    ASSERT_EQ(run.total, 2u);
    ASSERT_EQ(run.results.size(), 2u);
    EXPECT_EQ(run.results[0].position, 2u);
    EXPECT_EQ(run.results[0].key_id, 0u);
    EXPECT_EQ(run.results[1].position, 15u);
    EXPECT_EQ(run.results[1].key_id, 1u);
}

TEST(FindkeyPublicApiTest, RejectsEmptyInput) {
    const std::vector<std::string_view> keys = {"test"};

    for (const auto algorithm : {SCALAR, TEDDY, TEDDY_BASELINE}) {
        SCOPED_TRACE(::testing::Message()
                     << "algorithm=" << static_cast<int>(algorithm));
        const ApiRun run = run_findkey("", keys, algorithm);

        EXPECT_EQ(run.status, FINDKEY_ERR_BAD_ARGS);
        EXPECT_EQ(run.total, 0u);
        EXPECT_TRUE(run.results.empty());
    }

    const auto* empty_data = reinterpret_cast<const uint8_t*>("");
    const auto* key_data =
        reinterpret_cast<const uint8_t*>(keys.front().data());
    const size_t key_length = keys.front().size();
    findkey_teddy_stats stats{};
    findkey_timing timing{};
    int status = FINDKEY_OK;

    const size_t total =
        findkey_with_stats(empty_data, 0, &key_data, &key_length, 1, nullptr,
                           &stats, &status, &timing);

    EXPECT_EQ(status, FINDKEY_ERR_BAD_ARGS);
    EXPECT_EQ(total, 0u);
}

TEST(FindkeyDifferentialTest, MatchesScalarForJsonEdgeCases) {
    struct TestCase {
        std::string_view json;
        std::vector<std::string_view> keys;
        size_t expected_matches;
    };

    const std::vector<TestCase> cases = {
        {R"({"username":1,"id":"name","other":2})", {"name", "missing"}, 0},
        {R"({"escaped\"key":1,"backslash\\key":2})",
         {R"(escaped\"key)", R"(backslash\\key)"},
         2},
        {R"({"name":1,"name":2,"id":3})", {"name", "name", "id"}, 3},
        {"{\"spaced\" \n\t: 1}", {"spaced"}, 1},
    };

    for (const TestCase& test_case : cases) {
        SCOPED_TRACE(::testing::Message() << "JSON: " << test_case.json);

        const ApiRun scalar =
            run_findkey(test_case.json, test_case.keys, SCALAR);
        ASSERT_EQ(scalar.status, FINDKEY_OK);
        ASSERT_EQ(scalar.total, test_case.expected_matches);

        for (const auto suffix_mode : teddy::ALL_SUFFIX_MODES) {
            findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
            config.suffix_mode = suffix_mode;
            config.sigma = 4;
            expect_teddy_matchers_match(scalar, test_case.json, test_case.keys,
                                        &config);
        }
    }
}

TEST(FindkeyDifferentialTest, MatchesScalarWithDefaultTeddyConfiguration) {
    constexpr std::string_view json =
        R"({"alpha":1,"bravo":2,"value":"alpha"})";
    const std::vector<std::string_view> keys = {"alpha", "bravo"};

    const ApiRun scalar = run_findkey(json, keys, SCALAR);
    ASSERT_TRUE(expect_success(scalar));
    expect_teddy_matchers_match(scalar, json, keys);
}

TEST(FindkeyDifferentialTest, MatchesScalarWhenShortKeyCapsSigma) {
    constexpr std::string_view json = R"({"a":1,"ab":2,"abc":3,"abcd":4})";
    const std::vector<std::string_view> keys = {"a", "ab", "abc", "abcd"};

    for (const auto suffix_mode : teddy::ALL_SUFFIX_MODES) {
        findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
        config.suffix_mode = suffix_mode;
        config.sigma = 4;
        expect_teddy_matches_scalar(json, keys, config);
    }
}

TEST(FindkeyDifferentialTest, MatchesScalarAcrossBlockBoundaries) {
    const std::vector<std::string_view> keys = {"boundary"};

    // Key not aligned to a block of 16 bytes
    // e.g. for leading_spaces = 6:
    // |  <16 bytes>   |
    // |      {"boundar|
    // |y":1}
    for (const size_t leading_spaces : {6u, 7u, 22u, 23u}) {
        const std::string json =
            std::string(leading_spaces, ' ') + R"({"boundary":1})";
        SCOPED_TRACE(::testing::Message()
                     << "leading spaces: " << leading_spaces);

        for (const auto suffix_mode : teddy::ALL_SUFFIX_MODES) {
            findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
            config.suffix_mode = suffix_mode;
            config.sigma = 4;
            expect_teddy_matches_scalar(json, keys, config);
        }
    }
}

TEST(FindkeyDifferentialTest, MatchesScalarAcrossTeddyConfigurationMatrix) {
    const std::string json = load_json_fixture("configuration_matrix.json");
    const std::vector<std::string_view> keys = {
        "alpha", "bravo", "charlie", "delta",  "echo", "foxtrot",
        "golf",  "hotel", "india",   "juliet", "kilo", "lima",
    };

    const ApiRun scalar = run_findkey(json, keys, SCALAR);
    ASSERT_EQ(scalar.status, FINDKEY_OK);
    ASSERT_EQ(scalar.total, 13u);

    for (const auto config : teddy::all_teddy_configurations()) {
        SCOPED_TRACE(::testing::Message()
                     << "strategy="
                     << static_cast<int>(config.grouping.strategy)
                     << ", score=" << static_cast<int>(config.grouping.score)
                     << ", suffix_mode=" << static_cast<int>(config.suffix_mode)
                     << ", sigma=" << config.sigma);

        expect_teddy_matchers_match(scalar, json, keys, &config);
    }
}
