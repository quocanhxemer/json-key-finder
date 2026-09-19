#include "bench/bench_csv.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::vector<std::string> split_csv_row(std::string_view csv) {
    if (!csv.empty() && csv.back() == '\n') {
        csv.remove_suffix(1);
    }

    std::vector<std::string> columns;
    size_t begin = 0;
    while (begin <= csv.size()) {
        const size_t comma = csv.find(',', begin);
        if (comma == std::string_view::npos) {
            columns.emplace_back(csv.substr(begin));
            break;
        }
        columns.emplace_back(csv.substr(begin, comma - begin));
        begin = comma + 1;
    }
    return columns;
}

std::vector<std::string> bench_header() {
    std::ostringstream output;
    bench::write_bench_header(output);
    return split_csv_row(output.str());
}

std::vector<std::string> stats_header() {
    std::ostringstream output;
    bench::write_stats_header(output);
    return split_csv_row(output.str());
}

}  // namespace

TEST(BenchCsvTest, BenchHeaderMatchesBenchRowSchema) {
    const bench::KeyCase key_case{
        .json_path = "fixture.json",
        .key_type = keygen::KeyType::Highest,
        .num_keys = 7,
        .seed = 42,
    };
    findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    config.verification_strategy = TEDDY_VERIFY_HASH;

    const bench::BenchCsvRow row{
        .key_case = key_case,
        .actual_num_keys = 6,
        .algo = TEDDY_BASELINE,
        .teddy_config = config,
        .repeat_index = 2,
        .status = FINDKEY_OK,
        .total_found = 3,
        .timing = {.compile_ns = 11, .verifier_build_ns = 13, .match_ns = 17},
        .data_bytes = 17,
        .throughput_mib_s = 1.5,
        .end_to_end_throughput_mib_s = 2.5,
    };

    std::ostringstream output;
    bench::write_bench_row(output, row);
    const std::vector<std::string> header = bench_header();
    const std::vector<std::string> values = split_csv_row(output.str());

    ASSERT_EQ(header.size(), values.size());
    EXPECT_EQ(header.size(), 21u);
    EXPECT_EQ(header[14], "compile_ns");
    EXPECT_EQ(header[15], "verifier_build_ns");
    EXPECT_EQ(header[16], "match_ns");
    EXPECT_EQ(header[17], "total_ns");
    EXPECT_EQ(values[6], "hash");
    EXPECT_EQ(values[14], "11");
    EXPECT_EQ(values[15], "13");
    EXPECT_EQ(values[16], "17");
    EXPECT_EQ(values[17], "41");
    EXPECT_EQ(values[18], "17");
}

TEST(BenchCsvTest, StatsHeaderMatchesStatsRowSchemaWithoutTiming) {
    const bench::KeyCase key_case{
        .json_path = "fixture.json",
        .key_type = keygen::KeyType::Mixed,
        .num_keys = 19,
        .seed = 23,
    };
    const bench::StatsCsvRow row{
        .key_case = key_case,
        .actual_num_keys = 18,
        .metadata = {.sigma = 3, .num_groups = 4},
        .verifier_metadata = {.strategy = TEDDY_VERIFY_PLAIN_TRIE,
                              .trie_nodes = 29,
                              .hash_keys = 0},
        .repeat_index = 5,
        .status = FINDKEY_OK,
        .total_found = 7,
        .data_bytes = 37,
        .stats = {.prefilter_hit_lanes = 41,
                  .prefilter_hit_groups = 43,
                  .fp_type1_lanes = 47,
                  .fp_type1_groups = 53,
                  .fp_type2_lanes = 59,
                  .reject_bad_end_quote = 61,
                  .reject_invalid_quote = 67,
                  .reject_missing_colon = 71,
                  .reject_missing_open_quote = 73,
                  .reject_key_not_found = 79,
                  .exact_matches = 83},
        .hit_lane_ratio = 0.1,
        .avg_hit_groups_per_lane = 0.2,
        .exact_matches_per_hit_lane = 0.3,
        .fp_type1_lane_ratio = 0.4,
        .fp_type2_lane_ratio = 0.5,
    };

    std::ostringstream output;
    bench::write_stats_row(output, row);
    const std::vector<std::string> header = stats_header();
    const std::vector<std::string> values = split_csv_row(output.str());

    ASSERT_EQ(header.size(), values.size());
    EXPECT_EQ(header.size(), 34u);
    EXPECT_EQ(std::find(header.begin(), header.end(), "compile_ns"),
              header.end());
    EXPECT_EQ(std::find(header.begin(), header.end(), "verifier_build_ns"),
              header.end());
    EXPECT_EQ(std::find(header.begin(), header.end(), "match_ns"),
              header.end());
    EXPECT_EQ(std::find(header.begin(), header.end(), "total_ns"),
              header.end());
    EXPECT_EQ(std::find(header.begin(), header.end(), "max_key_len"),
              header.end());
    EXPECT_EQ(values[5], "plain_trie");
    EXPECT_EQ(header[17], "data_bytes");
    EXPECT_EQ(values[12], "29");
    EXPECT_EQ(values[17], "37");
    EXPECT_EQ(values[18], "41");
    EXPECT_EQ(values[28], "83");
}
