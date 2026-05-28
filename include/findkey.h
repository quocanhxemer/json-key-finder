#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FINDKEY_TEDDY_DEFAULT_SUFFIX_LENGTH 3
#define FINDKEY_TEDDY_MAX_SUFFIX_LENGTH 4

#define FINDKEY_TEDDY_MAX_SIGMA 5

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

struct findkey_timing {
    uint64_t compile_ns;
    uint64_t match_ns;
};

enum findkey_teddy_compile_grouping_strategy {
    TEDDY_COMPILE_PAPER_GREEDY = 0,
    TEDDY_COMPILE_PAPER_IMPROVED_GREEDY = 1,
    TEDDY_COMPILE_HASH = 2,
};

enum findkey_teddy_compile_hash_algorithm {
    TEDDY_HASH_STD = 0,
    TEDDY_HASH_ADLER32 = 1,
    TEDDY_HASH_CRC32 = 2,
    TEDDY_HASH_XXHASH = 3,
    TEDDY_HASH_FNV1A = 4,
};

enum findkey_teddy_suffix_mode {
    TEDDY_SUFFIX_RAW = 0,
    TEDDY_SUFFIX_QUOTED = 1,
};

struct findkey_teddy_config {
    enum findkey_teddy_compile_grouping_strategy grouping_strategy;

    // only used if grouping_strategy is TEDDY_COMPILE_HASH
    enum findkey_teddy_compile_hash_algorithm hash_algorithm;
    enum findkey_teddy_suffix_mode suffix_mode;

    int sigma;
};

#define FINDKEY_TEDDY_CONFIG_INIT                                  \
    {TEDDY_COMPILE_PAPER_GREEDY, TEDDY_HASH_STD, TEDDY_SUFFIX_RAW, \
     FINDKEY_TEDDY_DEFAULT_SUFFIX_LENGTH}

size_t findkey(const uint8_t* data,
               size_t len,
               const uint8_t* const* keys,
               const size_t* key_lens,
               size_t num_keys,
               enum findkey_algo algo,
               const struct findkey_teddy_config* teddy_config,
               struct findkey_result* out_results,
               size_t max_out_positions,
               int* out_status,
               struct findkey_timing* out_timing);

// statistics collection for teddy
size_t findkey_with_stats(const uint8_t* data,
                          size_t len,
                          const uint8_t* const* keys,
                          const size_t* key_lens,
                          size_t num_keys,
                          const struct findkey_teddy_config* teddy_config,
                          struct findkey_teddy_stats* teddy_stats,
                          int* out_status,
                          struct findkey_timing* out_timing);

#ifdef __cplusplus
}
#endif
