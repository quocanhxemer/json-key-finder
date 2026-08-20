#include "bench/bench_csv.h"

#include "core/findkey_options.h"

#include <cassert>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace bench {
namespace {

constexpr size_t BENCH_COLUMN_COUNT = 19;
constexpr size_t STATS_COLUMN_COUNT = 36;
constexpr size_t BENCH_TEDDY_COLUMN_COUNT = 4;

// RFC4180 CSV escaping
std::string csv_escape(std::string value) {
    bool needs_quotes = false;
    for (const char character : value) {
        if (character == ',' || character == '"' || character == '\n' ||
            character == '\r') {
            needs_quotes = true;
            break;
        }
    }

    if (!needs_quotes) {
        return value;
    }

    std::string escaped = "\"";
    for (const char character : value) {
        if (character == '"') {
            escaped += "\"\"";
        } else {
            escaped += character;
        }
    }
    escaped += '"';
    return escaped;
}

std::string to_string_double(double value) {
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<double>::max_digits10)
        << value;
    return out.str();
}

void append_empty_columns(std::vector<std::string>& row, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        row.push_back("");
    }
}

void write_csv_row(std::ofstream& output, const std::vector<std::string>& row) {
    for (size_t column_index = 0; column_index < row.size(); ++column_index) {
        if (column_index != 0) {
            output << ',';
        }
        output << csv_escape(row[column_index]);
    }
    output << '\n';
}

}  // namespace

void write_bench_header(std::ofstream& output) {
    const std::vector<std::string> header = {
        "json_path",
        "key_type",
        "requested_num_keys",
        "actual_num_keys",
        "seed",
        "algo",
        "grouping_strategy",
        "grouping_score",
        "suffix_mode",
        "requested_sigma",
        "repeat_index",
        "status",
        "total_found",
        "compile_ns",
        "match_ns",
        "total_ns",
        "data_bytes",
        "throughput_mib_s",
        "end_to_end_throughput_mib_s",
    };

    assert(header.size() == BENCH_COLUMN_COUNT);
    write_csv_row(output, header);
}

void write_stats_header(std::ofstream& output) {
    const std::vector<std::string> header = {
        "json_path",
        "key_type",
        "requested_num_keys",
        "actual_num_keys",
        "seed",
        "grouping_strategy",
        "grouping_score",
        "suffix_mode",
        "requested_sigma",
        "compiled_sigma",
        "num_groups",
        "dfa_nodes",
        "max_key_len",
        "repeat_index",
        "status",
        "total_found",
        "compile_ns",
        "match_ns",
        "total_ns",
        "data_bytes",
        "prefilter_hit_lanes",
        "prefilter_hit_groups",
        "fp_type1_lanes",
        "fp_type1_groups",
        "fp_type2_lanes",
        "reject_bad_end_quote",
        "reject_invalid_quote",
        "reject_missing_colon",
        "reject_missing_open_quote",
        "reject_key_not_found",
        "exact_matches",
        "hit_lane_ratio",
        "avg_hit_groups_per_lane",
        "exact_matches_per_hit_lane",
        "fp_type1_lane_ratio",
        "fp_type2_lane_ratio",
    };

    assert(header.size() == STATS_COLUMN_COUNT);
    write_csv_row(output, header);
}

void write_bench_row(std::ofstream& output, const BenchCsvRow& row) {
    const uint64_t total_ns = row.timing.compile_ns + row.timing.match_ns;
    std::vector<std::string> csv_row;
    csv_row.reserve(BENCH_COLUMN_COUNT);

    csv_row.push_back(row.key_case.json_path);
    csv_row.push_back(
        std::string(keygen::key_type_name(row.key_case.key_type)));
    csv_row.push_back(std::to_string(row.key_case.num_keys));
    csv_row.push_back(std::to_string(row.actual_num_keys));
    csv_row.push_back(std::to_string(row.key_case.seed));
    csv_row.push_back(std::string(findkey_options::algo_name(row.algo)));

    if (row.algo == SCALAR) {
        append_empty_columns(csv_row, BENCH_TEDDY_COLUMN_COUNT);
    } else {
        csv_row.push_back(std::string(findkey_options::grouping_strategy_name(
            row.teddy_config.grouping.strategy)));
        csv_row.push_back(std::string(findkey_options::grouping_score_name(
            row.teddy_config.grouping.score)));
        csv_row.push_back(std::string(
            findkey_options::suffix_mode_name(row.teddy_config.suffix_mode)));
        csv_row.push_back(std::to_string(row.teddy_config.sigma));
    }

    csv_row.push_back(std::to_string(row.repeat_index));
    csv_row.push_back(std::string(findkey_options::status_name(row.status)));
    csv_row.push_back(std::to_string(row.total_found));
    csv_row.push_back(std::to_string(row.timing.compile_ns));
    csv_row.push_back(std::to_string(row.timing.match_ns));
    csv_row.push_back(std::to_string(total_ns));
    csv_row.push_back(std::to_string(row.data_bytes));
    csv_row.push_back(to_string_double(row.throughput_mib_s));
    csv_row.push_back(to_string_double(row.end_to_end_throughput_mib_s));

    assert(csv_row.size() == BENCH_COLUMN_COUNT);
    write_csv_row(output, csv_row);
}

void write_stats_row(std::ofstream& output, const StatsCsvRow& row) {
    const uint64_t total_ns = row.timing.compile_ns + row.timing.match_ns;
    std::vector<std::string> csv_row;
    csv_row.reserve(STATS_COLUMN_COUNT);

    csv_row.push_back(row.key_case.json_path);
    csv_row.push_back(
        std::string(keygen::key_type_name(row.key_case.key_type)));
    csv_row.push_back(std::to_string(row.key_case.num_keys));
    csv_row.push_back(std::to_string(row.actual_num_keys));
    csv_row.push_back(std::to_string(row.key_case.seed));
    csv_row.push_back(std::string(findkey_options::grouping_strategy_name(
        row.teddy_config.grouping.strategy)));
    csv_row.push_back(std::string(
        findkey_options::grouping_score_name(row.teddy_config.grouping.score)));
    csv_row.push_back(std::string(
        findkey_options::suffix_mode_name(row.teddy_config.suffix_mode)));
    csv_row.push_back(std::to_string(row.teddy_config.sigma));
    csv_row.push_back(std::to_string(row.metadata.sigma));
    csv_row.push_back(std::to_string(row.metadata.num_groups));
    csv_row.push_back(std::to_string(row.dfa_metadata.nodes));
    csv_row.push_back(std::to_string(row.dfa_metadata.max_key_len));
    csv_row.push_back(std::to_string(row.repeat_index));
    csv_row.push_back(std::string(findkey_options::status_name(row.status)));
    csv_row.push_back(std::to_string(row.total_found));
    csv_row.push_back(std::to_string(row.timing.compile_ns));
    csv_row.push_back(std::to_string(row.timing.match_ns));
    csv_row.push_back(std::to_string(total_ns));
    csv_row.push_back(std::to_string(row.data_bytes));
    csv_row.push_back(std::to_string(row.stats.prefilter_hit_lanes));
    csv_row.push_back(std::to_string(row.stats.prefilter_hit_groups));
    csv_row.push_back(std::to_string(row.stats.fp_type1_lanes));
    csv_row.push_back(std::to_string(row.stats.fp_type1_groups));
    csv_row.push_back(std::to_string(row.stats.fp_type2_lanes));
    csv_row.push_back(std::to_string(row.stats.reject_bad_end_quote));
    csv_row.push_back(std::to_string(row.stats.reject_invalid_quote));
    csv_row.push_back(std::to_string(row.stats.reject_missing_colon));
    csv_row.push_back(std::to_string(row.stats.reject_missing_open_quote));
    csv_row.push_back(std::to_string(row.stats.reject_key_not_found));
    csv_row.push_back(std::to_string(row.stats.exact_matches));
    csv_row.push_back(to_string_double(row.hit_lane_ratio));
    csv_row.push_back(to_string_double(row.avg_hit_groups_per_lane));
    csv_row.push_back(to_string_double(row.exact_matches_per_hit_lane));
    csv_row.push_back(to_string_double(row.fp_type1_lane_ratio));
    csv_row.push_back(to_string_double(row.fp_type2_lane_ratio));

    assert(csv_row.size() == STATS_COLUMN_COUNT);
    write_csv_row(output, csv_row);
}

}  // namespace bench
