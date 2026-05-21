#include "cli/reporting.h"

#include <cstdio>

void print_teddy_compilation_stats(const TeddyCompilationMetadata& metadata) {
    std::printf("Teddy Compilation Stats:\n");
    std::printf("\tSigma: %d\n", metadata.sigma);
    std::printf("\tGroups: %d\n", metadata.num_groups);
    std::printf("\tDFA nodes: %zu\n", metadata.dfa_nodes);
    std::printf("\tMax key length: %zu\n", metadata.max_key_len);
    std::printf("\tTotal group score: %llu\n",
                static_cast<unsigned long long>(metadata.total_score));
}

void print_teddy_runtime_stats(const findkey_teddy_stats& teddy_stats,
                               size_t data_len) {
    const size_t scan_positions = data_len;
    const double hit_lane_ratio =
        scan_positions > 0
            ? static_cast<double>(teddy_stats.prefilter_hit_lanes) /
                  static_cast<double>(scan_positions)
            : 0.0;
    const double avg_hit_groups =
        teddy_stats.prefilter_hit_lanes > 0
            ? static_cast<double>(teddy_stats.prefilter_hit_groups) /
                  static_cast<double>(teddy_stats.prefilter_hit_lanes)
            : 0.0;
    const double exact_match_ratio =
        teddy_stats.prefilter_hit_lanes > 0
            ? static_cast<double>(teddy_stats.exact_matches) /
                  static_cast<double>(teddy_stats.prefilter_hit_lanes)
            : 0.0;
    const double fp_type1_ratio =
        teddy_stats.prefilter_hit_lanes > 0
            ? static_cast<double>(teddy_stats.fp_type1_lanes) /
                  static_cast<double>(teddy_stats.prefilter_hit_lanes)
            : 0.0;
    const double fp_type2_ratio =
        teddy_stats.prefilter_hit_lanes > 0
            ? static_cast<double>(teddy_stats.fp_type2_lanes) /
                  static_cast<double>(teddy_stats.prefilter_hit_lanes)
            : 0.0;

    std::printf("Teddy Runtime Stats:\n");
    std::printf("\tScan positions: %zu\n", scan_positions);
    std::printf("\tPrefilter hit lanes: %lu\n",
                teddy_stats.prefilter_hit_lanes);
    std::printf("\tPrefilter hit groups: %lu\n",
                teddy_stats.prefilter_hit_groups);
    std::printf("\tFP type 1 lanes: %lu\n", teddy_stats.fp_type1_lanes);
    std::printf("\tFP type 1 groups: %lu\n", teddy_stats.fp_type1_groups);
    std::printf("\tFP type 2 lanes: %lu\n", teddy_stats.fp_type2_lanes);
    std::printf("\tReject bad end quote: %lu\n",
                teddy_stats.reject_bad_end_quote);
    std::printf("\tReject invalid quote: %lu\n",
                teddy_stats.reject_invalid_quote);
    std::printf("\tReject missing colon: %lu\n",
                teddy_stats.reject_missing_colon);
    std::printf("\tReject missing open quote: %lu\n",
                teddy_stats.reject_missing_open_quote);
    std::printf("\tReject key not found: %lu\n",
                teddy_stats.reject_key_not_found);
    std::printf("\tExact matches: %lu\n", teddy_stats.exact_matches);
    std::printf("\tHit lane ratio: %.6f\n", hit_lane_ratio);
    std::printf("\tAvg hit groups per lane: %.6f\n", avg_hit_groups);
    std::printf("\tExact matches per hit lane: %.6f\n", exact_match_ratio);
    std::printf("\tFP type 1 lane ratio: %.6f\n", fp_type1_ratio);
    std::printf("\tFP type 2 lane ratio: %.6f\n", fp_type2_ratio);
}
