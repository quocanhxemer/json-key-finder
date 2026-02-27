#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum findkey_status {
    FINDKEY_OK = 0,
    FINDKEY_ERR_BAD_ARGS = 1,
    FINDKEY_TEDDY_NOT_SUPPORTED = 2,
    FINDKEY_ERR_UNKNOWN_ALGO = 3,
};

enum findkey_algo {
    SCALAR = 0,
    TEDDY = 1,
    TEDDY_BASELINE = 2,
};

struct findkey_result {
    size_t position;
    uint32_t key_id;
};

struct findkey_teddy_stats {
    uint64_t prefilter_hit_lanes;
    uint64_t prefilter_hit_groups;

    uint64_t fp_type1_lanes;
    uint64_t fp_type1_groups;
    uint64_t fp_type2_lanes;

    uint64_t reject_bad_end_quote;
    uint64_t reject_invalid_quote;
    uint64_t reject_missing_colon;
    uint64_t reject_missing_open_quote;
    uint64_t reject_key_not_found;

    uint64_t exact_matches;
};

enum findkey_teddy_compile_grouping_strategy {
    TEDDY_COMPILE_GREEDY = 0,  // strategy stated in the original teddy paper
    TEDDY_COMPILE_HASH = 1,
};

enum findkey_teddy_suffix_mode {
    TEDDY_SUFFIX_RAW = 0,
    TEDDY_SUFFIX_QUOTED = 1,
};

size_t findkey(const uint8_t* data,
               size_t len,
               const uint8_t* const* keys,
               const size_t* key_lens,
               size_t num_keys,
               enum findkey_algo algo,
               enum findkey_teddy_compile_grouping_strategy grouping_strategy,
               enum findkey_teddy_suffix_mode suffix_mode,
               struct findkey_result* out_results,
               size_t max_out_positions,
               int* out_status);

// statistics collection for teddy
size_t findkey_with_stats(
    const uint8_t* data,
    size_t len,
    const uint8_t* const* keys,
    const size_t* key_lens,
    size_t num_keys,
    enum findkey_teddy_compile_grouping_strategy grouping_strategy,
    enum findkey_teddy_suffix_mode suffix_mode,
    struct findkey_teddy_stats* teddy_stats,
    int* out_status);

#ifdef __cplusplus
}
#endif
