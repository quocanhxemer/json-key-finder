#include "bench/bench_runner.h"

#include "bench/bench_csv.h"
#include "teddy/teddy_common.h"

#include <vector>

namespace bench {
namespace {

TeddyCompilationMetadata compile_metadata(const PreparedKeys& keys,
                                          const TeddyConfigCase& config_case) {
    const TeddyCompilationData teddy_data =
        compile_teddy_data(keys.views, config_case.config);
    return get_teddy_compilation_metadata(teddy_data);
}

}  // namespace

void run_bench_case(std::ofstream& output,
                    std::string_view data,
                    const KeyCase& key_case,
                    const PreparedKeys& keys,
                    findkey_algo algo,
                    const std::optional<TeddyConfigCase>& teddy_config_case,
                    size_t repeat_count,
                    size_t warmup_count) {
    findkey_teddy_config teddy_config = FINDKEY_TEDDY_CONFIG_INIT;

    if (algo != SCALAR && teddy_config_case) {
        teddy_config = teddy_config_case->config;
    }

    constexpr size_t positions_capacity = 1024 * 1024;
    std::vector<findkey_result> positions(positions_capacity);
    const size_t total_iterations = repeat_count + warmup_count;

    for (size_t iteration = 0; iteration < total_iterations; ++iteration) {
        int status = FINDKEY_OK;
        findkey_timing timing = {};

        const size_t total_found =
            findkey(reinterpret_cast<const uint8_t*>(data.data()), data.size(),
                    keys.ptrs.data(), keys.lens.data(), keys.ptrs.size(), algo,
                    &teddy_config, positions.data(), positions.size(), &status,
                    &timing);

        if (iteration < warmup_count) {
            continue;
        }

        const uint64_t total_ns = timing.compile_ns + timing.match_ns;
        const double data_mib =
            static_cast<double>(data.size()) / (1024.0 * 1024.0);
        const double throughput = timing.match_ns > 0
                                      ? data_mib * 1'000'000'000.0 /
                                            static_cast<double>(timing.match_ns)
                                      : 0.0;
        const double end_to_end_throughput =
            total_ns > 0
                ? data_mib * 1'000'000'000.0 / static_cast<double>(total_ns)
                : 0.0;

        const size_t repeat_index = iteration - warmup_count;
        write_bench_row(output,
                        {key_case, keys.keys.size(), algo, teddy_config_case,
                         repeat_index, status, total_found, timing, data.size(),
                         throughput, end_to_end_throughput});
    }
}

void run_stats_case(std::ofstream& output,
                    std::string_view data,
                    const KeyCase& key_case,
                    const PreparedKeys& keys,
                    const TeddyConfigCase& teddy_config_case,
                    size_t repeat_count,
                    size_t warmup_count) {
    const TeddyCompilationMetadata metadata =
        compile_metadata(keys, teddy_config_case);

    const size_t total_iterations = repeat_count + warmup_count;
    for (size_t iteration = 0; iteration < total_iterations; ++iteration) {
        int status = FINDKEY_OK;
        findkey_timing timing = {};
        findkey_teddy_stats stats = {};

        const size_t total_found = findkey_with_stats(
            reinterpret_cast<const uint8_t*>(data.data()), data.size(),
            keys.ptrs.data(), keys.lens.data(), keys.ptrs.size(),
            &teddy_config_case.config, &stats, &status, &timing);

        if (iteration < warmup_count) {
            continue;
        }

        const double scan_positions = static_cast<double>(data.size());
        const double hit_lanes = static_cast<double>(stats.prefilter_hit_lanes);
        const double hit_lane_ratio =
            scan_positions > 0.0
                ? static_cast<double>(stats.prefilter_hit_lanes) /
                      scan_positions
                : 0.0;
        const double avg_hit_groups =
            hit_lanes > 0.0
                ? static_cast<double>(stats.prefilter_hit_groups) / hit_lanes
                : 0.0;
        const double exact_match_ratio =
            hit_lanes > 0.0
                ? static_cast<double>(stats.exact_matches) / hit_lanes
                : 0.0;
        const double fp_type1_ratio =
            hit_lanes > 0.0
                ? static_cast<double>(stats.fp_type1_lanes) / hit_lanes
                : 0.0;
        const double fp_type2_ratio =
            hit_lanes > 0.0
                ? static_cast<double>(stats.fp_type2_lanes) / hit_lanes
                : 0.0;

        const size_t repeat_index = iteration - warmup_count;
        write_stats_row(
            output, {key_case, keys.keys.size(), teddy_config_case, metadata,
                     repeat_index, status, total_found, timing, data.size(),
                     stats, hit_lane_ratio, avg_hit_groups, exact_match_ratio,
                     fp_type1_ratio, fp_type2_ratio});
    }
}

}  // namespace bench
