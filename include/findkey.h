#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FINDKEY_TEDDY_DEFAULT_REQUESTED_SIGMA 3
#define FINDKEY_TEDDY_MAX_REQUESTED_SIGMA 4

#define FINDKEY_TEDDY_MAX_COMPILED_SIGMA (FINDKEY_TEDDY_MAX_REQUESTED_SIGMA + 1)

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
    uint64_t verifier_build_ns;
    uint64_t match_ns;
};

enum findkey_teddy_compile_grouping_strategy {

    // Grouping strategy from the original paper
    TEDDY_COMPILE_GREEDY_PAPER_POLICY = 0,

    // Altered version that minimizes the score delta when merging groups
    TEDDY_COMPILE_GREEDY_MIN_DELTA = 1,

    // Groups suffixes into bucket according to their hash values
    TEDDY_COMPILE_HASH_STD = 2,
    TEDDY_COMPILE_HASH_ADLER32 = 3,
    TEDDY_COMPILE_HASH_CRC32 = 4,
    TEDDY_COMPILE_HASH_XXHASH = 5,
    TEDDY_COMPILE_HASH_FNV1A = 6,

    // Sorts suffixes lexicographically and distributes them
    // into groups in a round-robin fashion
    // Used as contrast to compare with TEDDY_COMPILE_SORTED_SUFFIX_PARTITION
    TEDDY_COMPILE_SORTED_SUFFIX_ROUND_ROBIN = 7,

    // Sorts suffixes lexicographically and groups them into contiguous ranges
    TEDDY_COMPILE_SORTED_SUFFIX_PARTITION = 8,

    // Sorts suffixes lexicographically
    // and finds the most optimal partition using dynamic programming
    TEDDY_COMPILE_SORTED_SUFFIX_OPTIMAL_PARTITION = 9,

    FINDKEY_TEDDY_COMPILE_GROUPING_STRATEGY_COUNT,
};

enum findkey_teddy_grouping_score {

    // Score model from the paper
    TEDDY_GROUPING_SCORE_PAPER = 0,

    // Score model from the paper but uses nibbles instead of bytes
    TEDDY_GROUPING_SCORE_PAPER_NIBBLE = 1,

    // Count the number of unique nibbles in each byte of the suffixes
    // then take their product as the score
    // Lower score should mean lower diversity of nibbles in one position,
    // which should lower false positive rate
    TEDDY_GROUPING_SCORE_NIBBLE_COUNT = 2,

    FINDKEY_TEDDY_GROUPING_SCORE_COUNT,
};

enum findkey_teddy_suffix_mode {

    // Suffixes are the last `sigma` bytes of the key
    TEDDY_SUFFIX_RAW = 0,

    // The closing quote is included in the suffix
    // for lower false positive rate
    TEDDY_SUFFIX_QUOTED = 1,

    FINDKEY_TEDDY_SUFFIX_MODE_COUNT,
};

enum findkey_teddy_verification_strategy {

    // Verification by hash table lookup
    TEDDY_VERIFY_HASH = 0,

    // Verification by a Trie for all keys
    TEDDY_VERIFY_PLAIN_TRIE = 1,

    // Each node contains a bitmask of the groups of the current key
    // Support earlier exit when the candidate group is not in the bitmask
    TEDDY_VERIFY_GROUP_MASK_TRIE = 2,

    // Each group has its own Trie
    TEDDY_VERIFY_PER_GROUP_TRIE = 3,

    // Each suffix has its own Trie
    TEDDY_VERIFY_PER_SUFFIX_TRIE = 4,

    FINDKEY_TEDDY_VERIFICATION_STRATEGY_COUNT,
};

struct findkey_teddy_grouping_config {
    enum findkey_teddy_compile_grouping_strategy strategy;
    enum findkey_teddy_grouping_score score;
};

struct findkey_teddy_config {
    struct findkey_teddy_grouping_config grouping;
    enum findkey_teddy_suffix_mode suffix_mode;

    // for quoted suffix mode, sigma is added one for the closing quote
    int sigma;
    enum findkey_teddy_verification_strategy verification_strategy;
};

#define FINDKEY_TEDDY_GROUPING_CONFIG_INIT \
    {TEDDY_COMPILE_GREEDY_PAPER_POLICY, TEDDY_GROUPING_SCORE_PAPER}

#define FINDKEY_TEDDY_CONFIG_INIT                          \
    {FINDKEY_TEDDY_GROUPING_CONFIG_INIT, TEDDY_SUFFIX_RAW, \
     FINDKEY_TEDDY_DEFAULT_REQUESTED_SIGMA, TEDDY_VERIFY_PLAIN_TRIE}

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
                          int* out_status);

#ifdef __cplusplus
}
#endif
