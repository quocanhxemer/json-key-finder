#include "findkey.h"
#include "matchers/matcher_scalar.h"
#include "matchers/matcher_teddy_baseline.h"

#if COMPILER_SUPPORTS_TEDDY
#include "matchers/matcher_teddy.h"
#endif

#include <algorithm>
#include <string_view>
#include <vector>

static inline bool bad_args(const uint8_t* data,
                            size_t len,
                            const uint8_t* const* keys,
                            const size_t* key_lens,
                            size_t num_keys,
                            struct findkey_result* out_results) {
    if ((!data && len != 0) || !keys || !key_lens || !out_results ||
        !num_keys) {
        return true;
    }
    for (size_t i = 0; i < num_keys; ++i) {
        if (!keys[i] && key_lens[i] != 0) {
            return true;
        }
        if (key_lens[i] == 0) {
            return true;
        }
    }
    return false;
}

extern "C" size_t findkey(const uint8_t* data,
                          size_t len,
                          const uint8_t* const* keys,
                          const size_t* key_lens,
                          size_t num_keys,
                          enum findkey_algo algo,
                          struct findkey_result* out_results,
                          size_t max_out_positions,
                          int* out_status) {
    if (out_status) {
        *out_status = FINDKEY_OK;
    }

    if (bad_args(data, len, keys, key_lens, num_keys, out_results)) {
        if (out_status) {
            *out_status = FINDKEY_ERR_BAD_ARGS;
        }
        return 0;
    }

    const char* str = reinterpret_cast<const char*>(data);
    std::string_view data_sv(str ? str : "", len);

    std::vector<std::string_view> key_svs;
    key_svs.reserve(num_keys);
    for (size_t i = 0; i < num_keys; ++i) {
        const char* k = reinterpret_cast<const char*>(keys[i]);
        key_svs.emplace_back(k, key_lens[i]);
    }

    std::vector<findkey_result> results;

    switch (algo) {
        case SCALAR:
            results = matcher_scalar(data_sv, key_svs);
            break;

        case TEDDY:
#if COMPILER_SUPPORTS_TEDDY
            results = matcher_teddy(data_sv, key_svs);
            break;
#else
            (void)out_results;
            (void)max_out_positions;

            if (out_status) {
                *out_status = FINDKEY_TEDDY_NOT_SUPPORTED;
            }
            return 0;
#endif
        case TEDDY_BASELINE:
            results = matcher_teddy_baseline(data_sv, key_svs);
            break;
        default:
            if (out_status) {
                *out_status = FINDKEY_ERR_UNKNOWN_ALGO;
            }
            return 0;
    }

    const size_t num_positions = std::min(results.size(), max_out_positions);

    for (size_t i = 0; i < num_positions; ++i) {
        out_results[i] = results[i];
    }

    return results.size();
}
