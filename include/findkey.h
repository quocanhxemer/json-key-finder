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

size_t findkey(const uint8_t* data,
               size_t len,
               const uint8_t* const* keys,
               const size_t* key_lens,
               size_t num_keys,
               enum findkey_algo algo,
               struct findkey_result* out_results,
               size_t max_out_positions,
               int* out_status);

#ifdef __cplusplus
}
#endif
