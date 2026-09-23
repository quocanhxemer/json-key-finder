#include "teddy/grouping/score.h"
#include "teddy/grouping/scores/nibble_count.h"
#include "teddy/grouping/scores/paper.h"
#include "teddy/grouping/scores/paper_nibble.h"
#include "teddy/suffix.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace {

using teddy::grouping::NibbleCountScore;
using teddy::grouping::PaperNibbleScore;
using teddy::grouping::PaperScore;

constexpr teddy::Suffix ABC_SUFFIX{'a', 'b', 'c'};
constexpr teddy::Suffix DEF_SUFFIX{'d', 'e', 'f'};
constexpr teddy::Suffix ZERO_FIRST_SUFFIX{0x00, 'a', 'b'};
constexpr teddy::Suffix NIBBLE_12_SUFFIX{0x12, 0x12, 0x12};
constexpr teddy::Suffix NIBBLE_24_SUFFIX{0x24, 0x24, 0x24};
constexpr teddy::Suffix NIBBLE_11_SUFFIX{0x11, 0x11, 0x11};
constexpr teddy::Suffix NIBBLE_22_SUFFIX{0x22, 0x22, 0x22};
constexpr teddy::Suffix NIBBLE_44_SUFFIX{0x44, 0x44, 0x44};
constexpr teddy::Suffix NIBBLE_88_SUFFIX{0x88, 0x88, 0x88};

// from the paper's examples
constexpr teddy::Suffix PAPER_SHARED_SUFFIX{'d', 'd', 'y'};
constexpr teddy::Suffix PAPER_DIFFERENT_SUFFIX{'m', 'm', 'y'};

struct ScoreCase {
    std::string_view name;
    std::vector<teddy::Suffix> suffixes;
    uint64_t expected_value;
};

struct MergeCase {
    std::string_view name;
    std::vector<teddy::Suffix> left_suffixes;
    std::vector<teddy::Suffix> right_suffixes;
    uint64_t expected_paper;
    uint64_t expected_paper_nibble;
    uint64_t expected_nibble_count;
};

template <teddy::grouping::GroupingScore ScoreModel>
ScoreModel score_suffixes(const std::vector<teddy::Suffix>& suffixes) {
    ScoreModel score;
    for (const auto& suffix : suffixes) {
        score.add(suffix);
    }
    return score;
}

template <teddy::grouping::GroupingScore ScoreModel>
uint64_t score_after_merge(const std::vector<teddy::Suffix>& left_suffixes,
                           const std::vector<teddy::Suffix>& right_suffixes) {
    ScoreModel left = score_suffixes<ScoreModel>(left_suffixes);
    const ScoreModel right = score_suffixes<ScoreModel>(right_suffixes);
    left.merge(right);
    return left.value();
}

template <teddy::grouping::GroupingScore ScoreModel>
void expect_score_cases(const std::vector<ScoreCase>& cases) {
    for (const auto& test_case : cases) {
        SCOPED_TRACE(::testing::Message() << "case: " << test_case.name);
        const ScoreModel score = score_suffixes<ScoreModel>(test_case.suffixes);
        EXPECT_EQ(score.value(), test_case.expected_value);
    }
}

template <teddy::grouping::GroupingScore ScoreModel>
void expect_merge_case(const MergeCase& test_case,
                       const uint64_t expected_value) {
    ScoreModel left = score_suffixes<ScoreModel>(test_case.left_suffixes);
    ScoreModel right = score_suffixes<ScoreModel>(test_case.right_suffixes);
    const uint64_t right_value_before_merge = right.value();

    ScoreModel incremental = left;
    for (const auto& suffix : test_case.right_suffixes) {
        incremental.add(suffix);
    }

    left.merge(right);

    EXPECT_EQ(left.value(), expected_value);
    EXPECT_EQ(incremental.value(), expected_value);
    EXPECT_EQ(right.value(), right_value_before_merge);
}

template <template <int> class ScoreModel, int Sigma>
    requires teddy::grouping::GroupingScore<ScoreModel<Sigma>>
void expect_score_model_respects_sigma() {
    static_assert(Sigma > 0);
    static_assert(Sigma < FINDKEY_TEDDY_MAX_COMPILED_SIGMA,
                  "The test needs at least one byte after sigma");

    // Construct 2 identical Suffix-es with different trailing bytes
    // which are to be ignored
    teddy::Suffix first{};
    teddy::Suffix second{};
    for (std::size_t i = 0; i < first.size(); ++i) {
        if (i < static_cast<std::size_t>(Sigma)) {
            const auto byte = static_cast<uint8_t>(0x11 + i);
            first[i] = byte;
            second[i] = byte;
        } else {
            first[i] = 0x55;
            second[i] = 0xAA;
        }
    }

    ScoreModel<Sigma> score;
    score.add(first);
    const uint64_t score_before_inactive_bytes_change = score.value();

    score.add(second);

    EXPECT_EQ(score.value(), score_before_inactive_bytes_change);
}

template <int Sigma>
void expect_all_score_models_respect_sigma() {
    {
        SCOPED_TRACE("score model: paper");
        expect_score_model_respects_sigma<PaperScore, Sigma>();
    }
    {
        SCOPED_TRACE("score model: paper nibble");
        expect_score_model_respects_sigma<PaperNibbleScore, Sigma>();
    }
    {
        SCOPED_TRACE("score model: nibble count");
        expect_score_model_respects_sigma<NibbleCountScore, Sigma>();
    }
}

static_assert(teddy::grouping::GroupingScore<PaperScore<3>>);
static_assert(teddy::grouping::GroupingScore<PaperNibbleScore<3>>);
static_assert(teddy::grouping::GroupingScore<NibbleCountScore<3>>);

TEST(TeddyScoreModelsTest, DefaultConstructedGroupsHaveZeroScore) {
    EXPECT_EQ(PaperScore<3>{}.value(), 0u);
    EXPECT_EQ(PaperNibbleScore<3>{}.value(), 0u);
    EXPECT_EQ(NibbleCountScore<3>{}.value(), 0u);
}

TEST(TeddyPaperScoreTest, CalculatesExpectedScores) {
    const std::vector<ScoreCase> cases = {
        {"single suffix", {ABC_SUFFIX}, 36},
        {"duplicate suffix", {ABC_SUFFIX, ABC_SUFFIX}, 36},
        {"distinct suffixes", {ABC_SUFFIX, DEF_SUFFIX}, 100},
        {"zero bit position", {ZERO_FIRST_SUFFIX}, 0},
        {"paper figure 6 before mommy",
         {PAPER_SHARED_SUFFIX, PAPER_SHARED_SUFFIX, PAPER_SHARED_SUFFIX},
         45},
        {"paper figure 6 after mommy",
         {PAPER_SHARED_SUFFIX, PAPER_SHARED_SUFFIX, PAPER_SHARED_SUFFIX,
          PAPER_DIFFERENT_SUFFIX},
         125},
    };

    expect_score_cases<PaperScore<3>>(cases);
}

TEST(TeddyPaperNibbleScoreTest, CalculatesExpectedScores) {
    const std::vector<ScoreCase> cases = {
        {"single suffix", {ABC_SUFFIX}, 16},
        {"duplicate suffix", {ABC_SUFFIX, ABC_SUFFIX}, 16},
        {"distinct suffixes", {ABC_SUFFIX, DEF_SUFFIX}, 144},
        {"distinct high and low nibbles",
         {NIBBLE_12_SUFFIX, NIBBLE_24_SUFFIX},
         64},
        {"zero nibble position", {ZERO_FIRST_SUFFIX}, 0},
    };

    expect_score_cases<PaperNibbleScore<3>>(cases);
}

TEST(TeddyNibbleCountScoreTest, CalculatesExpectedScores) {
    const std::vector<ScoreCase> cases = {
        {"single suffix", {ABC_SUFFIX}, 1},
        {"duplicate suffix", {ABC_SUFFIX, ABC_SUFFIX}, 1},
        {"distinct suffixes", {ABC_SUFFIX, DEF_SUFFIX}, 8},
        {"distinct high and low nibbles",
         {NIBBLE_12_SUFFIX, NIBBLE_24_SUFFIX},
         64},
        {"zero is a distinct nibble value", {ZERO_FIRST_SUFFIX}, 1},
    };

    expect_score_cases<NibbleCountScore<3>>(cases);
}

TEST(TeddyScoreModelsTest, MergeMatchesScoringTheCombinedSuffixes) {
    const std::vector<MergeCase> cases = {
        {"single-suffix groups", {ABC_SUFFIX}, {DEF_SUFFIX}, 100, 144, 8},
        {"pre-aggregated groups",
         {NIBBLE_11_SUFFIX, NIBBLE_22_SUFFIX},
         {NIBBLE_44_SUFFIX, NIBBLE_88_SUFFIX},
         512,
         4096,
         4096},
        {"empty right group", {ABC_SUFFIX}, {}, 36, 16, 1},
        {"empty left group", {}, {DEF_SUFFIX}, 48, 32, 1},
    };

    for (const auto& test_case : cases) {
        SCOPED_TRACE(::testing::Message() << "case: " << test_case.name);

        {
            SCOPED_TRACE("score model: paper");
            expect_merge_case<PaperScore<3>>(test_case,
                                             test_case.expected_paper);
        }
        {
            SCOPED_TRACE("score model: paper nibble");
            expect_merge_case<PaperNibbleScore<3>>(
                test_case, test_case.expected_paper_nibble);
        }
        {
            SCOPED_TRACE("score model: nibble count");
            expect_merge_case<NibbleCountScore<3>>(
                test_case, test_case.expected_nibble_count);
        }
    }
}

TEST(TeddyScoreModelsTest, NibbleCountDistinguishesGroupingQuality) {
    struct TestCase {
        std::string_view name;
        std::vector<teddy::Suffix> base;
        std::vector<teddy::Suffix> sensible;
        std::vector<teddy::Suffix> poor;
        uint64_t expected_paper;
        uint64_t expected_paper_nibble;
        uint64_t expected_sensible_nibble_count;
        uint64_t expected_poor_nibble_count;
    };

    const std::vector<TestCase> cases = {
        {"shared high nibbles",
         {{'A', 'A', 'A'}},
         {{'C', 'C', 'A'}, {'C', 'A', 'C'}},
         {{'B', 'B', 'B'}, {'C', 'C', 'C'}},
         27,
         8,
         8,
         27},
        {"distinct high and low nibbles",
         {{'U', 'U', 'U'}},
         {{'w', 'w', 'U'}, {'w', 'U', 'w'}},
         {{'f', 'f', 'f'}, {'w', 'w', 'w'}},
         216,
         729,
         64,
         729},
    };

    for (const auto& test_case : cases) {
        SCOPED_TRACE(::testing::Message() << "case: " << test_case.name);
        EXPECT_EQ(score_after_merge<PaperScore<3>>(test_case.base,
                                                   test_case.sensible),
                  test_case.expected_paper);
        EXPECT_EQ(
            score_after_merge<PaperScore<3>>(test_case.base, test_case.poor),
            test_case.expected_paper);
        EXPECT_EQ(score_after_merge<PaperNibbleScore<3>>(test_case.base,
                                                         test_case.sensible),
                  test_case.expected_paper_nibble);
        EXPECT_EQ(score_after_merge<PaperNibbleScore<3>>(test_case.base,
                                                         test_case.poor),
                  test_case.expected_paper_nibble);
        EXPECT_EQ(score_after_merge<NibbleCountScore<3>>(test_case.base,
                                                         test_case.sensible),
                  test_case.expected_sensible_nibble_count);
        EXPECT_EQ(score_after_merge<NibbleCountScore<3>>(test_case.base,
                                                         test_case.poor),
                  test_case.expected_poor_nibble_count);
    }
}

TEST(TeddyScoreModelsTest, IgnoresSuffixBytesAfterSigma) {
    {
        SCOPED_TRACE("sigma: 1");
        expect_all_score_models_respect_sigma<1>();
    }
    {
        SCOPED_TRACE("sigma: 2");
        expect_all_score_models_respect_sigma<2>();
    }
    {
        SCOPED_TRACE("sigma: 4");
        expect_all_score_models_respect_sigma<4>();
    }
}

}  // namespace
