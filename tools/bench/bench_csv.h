#pragma once

#include "bench/bench_args.h"
#include "findkey.h"
#include "teddy/teddy_compile.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <optional>

namespace bench {

struct BenchCsvRow {
    const KeyCase& key_case;
    size_t actual_num_keys = 0;
    findkey_algo algo = SCALAR;
    std::optional<TeddyConfigCase> teddy_config;
    size_t repeat_index = 0;
    int status = FINDKEY_OK;
    size_t total_found = 0;
    findkey_timing timing = {};
    size_t data_bytes = 0;
    double throughput_mib_s = 0.0;
    double end_to_end_throughput_mib_s = 0.0;
};

struct StatsCsvRow {
    const KeyCase& key_case;
    size_t actual_num_keys = 0;
    TeddyConfigCase teddy_config;
    TeddyCompilationMetadata metadata = {};
    size_t repeat_index = 0;
    int status = FINDKEY_OK;
    size_t total_found = 0;
    findkey_timing timing = {};
    size_t data_bytes = 0;
    findkey_teddy_stats stats = {};
    double hit_lane_ratio = 0.0;
    double avg_hit_groups_per_lane = 0.0;
    double exact_matches_per_hit_lane = 0.0;
    double fp_type1_lane_ratio = 0.0;
    double fp_type2_lane_ratio = 0.0;
};

void write_bench_header(std::ofstream& output);

void write_stats_header(std::ofstream& output);

void write_bench_row(std::ofstream& output, const BenchCsvRow& row);

void write_stats_row(std::ofstream& output, const StatsCsvRow& row);

}  // namespace bench
