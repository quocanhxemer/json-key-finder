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
};

size_t findkey_scalar(const uint8_t* data,
                      size_t len,
                      const uint8_t* key,
                      size_t key_len,
                      size_t* out_positions,
                      size_t max_out_positions,
                      int* out_status);

size_t findkey_teddy(const uint8_t* data,
                      size_t len,
                      const uint8_t* key,
                      size_t key_len,
                      size_t* out_positions,
                      size_t max_out_positions,
                      int* out_status);

#ifdef __cplusplus
}
#endif
