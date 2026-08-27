#include "utils.h"

#include "core/findkey_error.h"
#include "teddy/suffix.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <exception>
#include <string>
#include <string_view>
#include <vector>

namespace {

findkey_teddy_config make_suffix_config(findkey_teddy_suffix_mode suffix_mode,
                                        int sigma) {
    findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    config.suffix_mode = suffix_mode;
    config.sigma = sigma;
    return config;
}

void expect_suffixes(const teddy::SuffixSet& actual,
                     const teddy::SuffixSet& expected) {
    ASSERT_EQ(actual.sigma, expected.sigma);
    EXPECT_EQ(actual.end_quote_offset, expected.end_quote_offset);
    ASSERT_EQ(actual.data.size(), expected.data.size());

    for (size_t index = 0; index < expected.data.size(); ++index) {
        SCOPED_TRACE(::testing::Message() << "suffix index " << index);
        EXPECT_EQ(actual.data[index], expected.data[index]);
    }
}

void expect_invalid_suffix_input(const std::vector<std::string_view>& keys,
                                 const findkey_teddy_config& config,
                                 std::string_view expected_message) {
    try {
        (void)teddy::prepare_suffixes(keys, config);
        FAIL() << "Expected prepare_suffixes() to reject its input";
    } catch (const FindkeyError& error) {
        EXPECT_EQ(error.code(), FindkeyErrorCode::INVALID_ARGUMENT);
        EXPECT_EQ(error.what(), expected_message);
    } catch (const std::exception& error) {
        FAIL() << "Expected FindkeyError, but caught: " << error.what();
    } catch (...) {
        FAIL() << "Expected FindkeyError, but caught an unknown exception";
    }
}

}  // namespace

TEST(TeddySuffixPreparationTest, RawModeUsesTrailingKeyBytes) {
    const std::vector<std::string_view> keys = {"alpha", "beta"};
    const findkey_teddy_config config = make_suffix_config(TEDDY_SUFFIX_RAW, 3);

    const teddy::SuffixSet prepared = teddy::prepare_suffixes(keys, config);
    const teddy::SuffixSet expected{
        .sigma = 3,
        .end_quote_offset = 1,
        .data =
            {
                teddy::Suffix{'p', 'h', 'a'},
                teddy::Suffix{'e', 't', 'a'},
            },
    };

    expect_suffixes(prepared, expected);
}

TEST(TeddySuffixPreparationTest, QuotedModeAppendsTheClosingQuote) {
    const std::vector<std::string_view> keys = {"alpha", "beta"};
    const findkey_teddy_config config =
        make_suffix_config(TEDDY_SUFFIX_QUOTED, 3);

    const teddy::SuffixSet prepared = teddy::prepare_suffixes(keys, config);
    const teddy::SuffixSet expected{
        .sigma = 4,
        .end_quote_offset = 0,
        .data =
            {
                teddy::Suffix{'p', 'h', 'a', '"'},
                teddy::Suffix{'e', 't', 'a', '"'},
            },
    };

    expect_suffixes(prepared, expected);
}

TEST(TeddySuffixPreparationTest, CapsSigmaAtTheShortestVirtualKey) {
    const std::vector<std::string_view> keys = {"alphabet", "a"};

    const teddy::SuffixSet raw =
        teddy::prepare_suffixes(keys, make_suffix_config(TEDDY_SUFFIX_RAW, 4));
    const teddy::SuffixSet expected_raw{
        .sigma = 1,
        .end_quote_offset = 1,
        .data =
            {
                teddy::Suffix{'t'},
                teddy::Suffix{'a'},
            },
    };
    expect_suffixes(raw, expected_raw);

    const teddy::SuffixSet quoted = teddy::prepare_suffixes(
        keys, make_suffix_config(TEDDY_SUFFIX_QUOTED, 4));
    const teddy::SuffixSet expected_quoted{
        .sigma = 2,
        .end_quote_offset = 0,
        .data =
            {
                teddy::Suffix{'t', '"'},
                teddy::Suffix{'a', '"'},
            },
    };
    expect_suffixes(quoted, expected_quoted);
}

TEST(TeddySuffixPreparationTest, SupportsTheMaximumRequestedSigma) {
    const std::vector<std::string_view> keys = {"alphabet"};

    const teddy::SuffixSet raw = teddy::prepare_suffixes(
        keys,
        make_suffix_config(TEDDY_SUFFIX_RAW, FINDKEY_TEDDY_MAX_SUFFIX_LENGTH));
    const teddy::SuffixSet expected_raw{
        .sigma = 4,
        .end_quote_offset = 1,
        .data = {teddy::Suffix{'a', 'b', 'e', 't'}},
    };
    expect_suffixes(raw, expected_raw);

    const teddy::SuffixSet quoted = teddy::prepare_suffixes(
        keys, make_suffix_config(TEDDY_SUFFIX_QUOTED,
                                 FINDKEY_TEDDY_MAX_SUFFIX_LENGTH));
    const teddy::SuffixSet expected_quoted{
        .sigma = 5,
        .end_quote_offset = 0,
        .data = {teddy::Suffix{'a', 'b', 'e', 't', '"'}},
    };
    expect_suffixes(quoted, expected_quoted);
}

TEST(TeddySuffixPreparationTest, DeduplicatesEqualSuffixesInFirstSeenOrder) {
    const std::vector<std::string_view> keys = {
        "alpha",
        "omega",
        "zalpha",
        "mega",
    };

    const teddy::SuffixSet raw =
        teddy::prepare_suffixes(keys, make_suffix_config(TEDDY_SUFFIX_RAW, 4));
    const teddy::SuffixSet expected_raw{
        .sigma = 4,
        .end_quote_offset = 1,
        .data =
            {
                teddy::Suffix{'l', 'p', 'h', 'a'},
                teddy::Suffix{'m', 'e', 'g', 'a'},
            },
    };
    expect_suffixes(raw, expected_raw);

    const teddy::SuffixSet quoted = teddy::prepare_suffixes(
        keys, make_suffix_config(TEDDY_SUFFIX_QUOTED, 4));
    const teddy::SuffixSet expected_quoted{
        .sigma = 5,
        .end_quote_offset = 0,
        .data =
            {
                teddy::Suffix{'l', 'p', 'h', 'a', '"'},
                teddy::Suffix{'m', 'e', 'g', 'a', '"'},
            },
    };
    expect_suffixes(quoted, expected_quoted);
}

TEST(TeddySuffixPreparationTest, PreservesNonAsciiBytes) {
    const std::string key = "caf\xC3\xA9";
    const std::vector<std::string_view> keys = {key};
    const findkey_teddy_config config = make_suffix_config(TEDDY_SUFFIX_RAW, 2);

    const teddy::SuffixSet prepared = teddy::prepare_suffixes(keys, config);
    const teddy::SuffixSet expected{
        .sigma = 2,
        .end_quote_offset = 1,
        .data = {teddy::Suffix{0xC3, 0xA9}},
    };

    expect_suffixes(prepared, expected);
}

TEST(TeddySuffixPreparationTest, RejectsAnEmptyKeyList) {
    const std::vector<std::string_view> keys;
    const findkey_teddy_config config = make_suffix_config(TEDDY_SUFFIX_RAW, 3);

    expect_invalid_suffix_input(keys, config,
                                "Teddy requires at least one key");
}

TEST(TeddySuffixPreparationTest, RejectsOutOfRangeSigma) {
    const std::vector<std::string_view> keys = {"alpha"};

    for (const int sigma : {-1, 0, FINDKEY_TEDDY_MAX_SUFFIX_LENGTH + 1}) {
        SCOPED_TRACE(::testing::Message() << "sigma=" << sigma);
        const findkey_teddy_config config =
            make_suffix_config(TEDDY_SUFFIX_RAW, sigma);
        expect_invalid_suffix_input(keys, config,
                                    "Teddy suffix length is out of range");
    }
}

TEST(TeddySuffixPreparationTest, RejectsAnUnknownSuffixMode) {
    const std::vector<std::string_view> keys = {"alpha"};
    const auto unknown_mode =
        static_cast<findkey_teddy_suffix_mode>(FINDKEY_TEDDY_SUFFIX_MODE_COUNT);
    const findkey_teddy_config config = make_suffix_config(unknown_mode, 3);

    expect_invalid_suffix_input(keys, config, "Unknown Teddy suffix mode");
}

TEST(TeddySuffixPreparationTest, RejectsAnEmptyKey) {
    const std::vector<std::string_view> keys = {"alpha", ""};
    const findkey_teddy_config config = make_suffix_config(TEDDY_SUFFIX_RAW, 3);

    expect_invalid_suffix_input(keys, config, "Teddy keys must not be empty");
}
