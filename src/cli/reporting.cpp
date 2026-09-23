#include "cli/reporting.h"

#include "core/findkey_options.h"

#include <iostream>

void print_compilation_stats(
    const teddy::CompilationMetadata& teddy_metadata,
    const teddy::VerifierCompilationMetadata& metadata) {
    std::cout << "Compilation Stats:\n";
    std::cout << "\tCompiled sigma: " << teddy_metadata.sigma << '\n';
    std::cout << "\tGroups: " << teddy_metadata.num_groups << '\n';
    std::cout << "\tVerification strategy: "
              << findkey_options::verification_strategy_name(metadata.strategy)
              << '\n';
    std::cout << "\tVerifier size: " << metadata.verifier_size_bytes
              << " bytes ("
              << static_cast<double>(metadata.verifier_size_bytes) /
                     (1024.0 * 1024.0)
              << " MiB)\n";
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

    std::cout << "Teddy Runtime Stats:\n";
    std::cout << "\tScan positions: " << scan_positions << '\n';
    std::cout << "\tPrefilter hit lanes: " << teddy_stats.prefilter_hit_lanes
              << '\n';
    std::cout << "\tPrefilter hit groups: " << teddy_stats.prefilter_hit_groups
              << '\n';
    std::cout << "\tFP type 1 lanes: " << teddy_stats.fp_type1_lanes << '\n';
    std::cout << "\tFP type 1 groups: " << teddy_stats.fp_type1_groups << '\n';
    std::cout << "\tFP type 2 lanes: " << teddy_stats.fp_type2_lanes << '\n';
    std::cout << "\tReject bad end quote: " << teddy_stats.reject_bad_end_quote
              << '\n';
    std::cout << "\tReject invalid quote: " << teddy_stats.reject_invalid_quote
              << '\n';
    std::cout << "\tReject missing colon: " << teddy_stats.reject_missing_colon
              << '\n';
    std::cout << "\tReject missing open quote: "
              << teddy_stats.reject_missing_open_quote << '\n';
    std::cout << "\tReject key not found: " << teddy_stats.reject_key_not_found
              << '\n';
    std::cout << "\tExact matches: " << teddy_stats.exact_matches << '\n';
    std::cout << "\tHit lane ratio: " << hit_lane_ratio << '\n';
    std::cout << "\tAvg hit groups per lane: " << avg_hit_groups << '\n';
    std::cout << "\tExact matches per hit lane: " << exact_match_ratio << '\n';
    std::cout << "\tFP type 1 lane ratio: " << fp_type1_ratio << '\n';
    std::cout << "\tFP type 2 lane ratio: " << fp_type2_ratio << '\n';
}
