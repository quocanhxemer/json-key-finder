#include "findkey.h"
#include "matcher_scalar.h"

#if COMPILER_SUPPORTS_TEDDY
#include "matcher_teddy.h"
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

extern "C" size_t findkey_scalar(const uint8_t* data,
                                 size_t len,
                                 const uint8_t* const* keys,
                                 const size_t* key_lens,
                                 size_t num_keys,
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

    std::vector<findkey_result> results = matcher_scalar(data_sv, key_svs);

    const size_t num_positions = std::min(results.size(), max_out_positions);

    for (size_t i = 0; i < num_positions; ++i) {
        out_results[i] = results[i];
    }

    return results.size();
}

extern "C" size_t findkey_teddy(const uint8_t* data,
                                size_t len,
                                const uint8_t* const* keys,
                                const size_t* key_lens,
                                size_t num_keys,
                                struct findkey_result* out_results,
                                size_t max_out_positions,
                                int* out_status) {
#if !COMPILER_SUPPORTS_TEDDY
    if (out_status) {
        *out_status = FINDKEY_TEDDY_NOT_SUPPORTED;
        (void)data;
        (void)len;
        (void)keys;
        (void)key_lens;
        (void)num_keys;
        (void)out_results;
        (void)max_out_positions;
        return 0;
    }
#else
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

    std::vector<findkey_result> results = matcher_teddy(data_sv, key_svs);

    const size_t num_positions = std::min(results.size(), max_out_positions);

    for (size_t i = 0; i < num_positions; ++i) {
        out_results[i] = results[i];
    }

    return results.size();
#endif
}
