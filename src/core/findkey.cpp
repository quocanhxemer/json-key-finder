#include "findkey.h"
#include "matchers/matcher_scalar.h"
#include "matchers/matcher_teddy_baseline.h"

#if COMPILER_SUPPORTS_TEDDY
#include "matchers/matcher_teddy.h"
#endif

#include <algorithm>
#include <chrono>
#include <string_view>
#include <utility>
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

static inline bool bad_args_stats(const uint8_t* data,
                                  size_t len,
                                  const uint8_t* const* keys,
                                  const size_t* key_lens,
                                  size_t num_keys,
                                  struct findkey_teddy_stats* teddy_stats) {
    if ((!data && len != 0) || !keys || !key_lens || !num_keys ||
        !teddy_stats) {
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

template <typename Fn>
static uint64_t measure_ns(Fn&& task) {
    const auto start = std::chrono::steady_clock::now();
    std::forward<Fn>(task)();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
        .count();
}

extern "C" size_t findkey(const uint8_t* data,
                          size_t len,
                          const uint8_t* const* keys,
                          const size_t* key_lens,
                          size_t num_keys,
                          enum findkey_algo algo,
                          const struct findkey_teddy_config* teddy_config,
                          struct findkey_result* out_results,
                          size_t max_out_positions,
                          int* out_status,
                          struct findkey_timing* out_timing) {
    if (out_status) {
        *out_status = FINDKEY_OK;
    }
    if (out_timing) {
        *out_timing = {};
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
    const findkey_teddy_config default_teddy_config = FINDKEY_TEDDY_CONFIG_INIT;
    const findkey_teddy_config& config =
        teddy_config ? *teddy_config : default_teddy_config;

    switch (algo) {
        case SCALAR: {
            if (out_timing) {
                out_timing->match_ns = measure_ns(
                    [&] { results = matcher_scalar(data_sv, key_svs); });
            } else {
                results = matcher_scalar(data_sv, key_svs);
            }
            break;
        }

        case TEDDY:
#if COMPILER_SUPPORTS_TEDDY
        {
            TeddyCompilationData teddy_data;
            if (out_timing) {
                out_timing->compile_ns = measure_ns(
                    [&] { teddy_data = compile_teddy_data(key_svs, config); });
                out_timing->match_ns = measure_ns(
                    [&] { results = matcher_teddy(data_sv, teddy_data); });
            } else {
                teddy_data = compile_teddy_data(key_svs, config);
                results = matcher_teddy(data_sv, teddy_data);
            }
            break;
        }
#else
            (void)config;
            (void)out_results;
            (void)max_out_positions;
            (void)out_timing;

            if (out_status) {
                *out_status = FINDKEY_TEDDY_NOT_SUPPORTED;
            }
            return 0;
#endif
        case TEDDY_BASELINE: {
            TeddyCompilationData teddy_data;
            if (out_timing) {
                out_timing->compile_ns = measure_ns(
                    [&] { teddy_data = compile_teddy_data(key_svs, config); });
                out_timing->match_ns = measure_ns([&] {
                    results =
                        matcher_teddy_baseline(data_sv, key_svs, teddy_data);
                });
            } else {
                teddy_data = compile_teddy_data(key_svs, config);
                results = matcher_teddy_baseline(data_sv, key_svs, teddy_data);
            }
            break;
        }
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

extern "C" size_t findkey_with_stats(
    const uint8_t* data,
    size_t len,
    const uint8_t* const* keys,
    const size_t* key_lens,
    size_t num_keys,
    const struct findkey_teddy_config* teddy_config,
    struct findkey_teddy_stats* teddy_stats,
    int* out_status,
    struct findkey_timing* out_timing) {
    if (out_status) {
        *out_status = FINDKEY_OK;
    }
    if (out_timing) {
        *out_timing = {};
    }

    if (bad_args_stats(data, len, keys, key_lens, num_keys, teddy_stats)) {
        if (out_status) {
            *out_status = FINDKEY_ERR_BAD_ARGS;
        }
        return 0;
    }

    *teddy_stats = {};

    const char* str = reinterpret_cast<const char*>(data);
    std::string_view data_sv(str ? str : "", len);

    std::vector<std::string_view> key_svs;
    key_svs.reserve(num_keys);
    for (size_t i = 0; i < num_keys; ++i) {
        const char* k = reinterpret_cast<const char*>(keys[i]);
        key_svs.emplace_back(k, key_lens[i]);
    }
    const findkey_teddy_config default_teddy_config = FINDKEY_TEDDY_CONFIG_INIT;
    const findkey_teddy_config& config =
        teddy_config ? *teddy_config : default_teddy_config;

    TeddyCompilationData teddy_data;
    std::vector<findkey_result> results;
    if (out_timing) {
        out_timing->compile_ns = measure_ns(
            [&] { teddy_data = compile_teddy_data(key_svs, config); });
        out_timing->match_ns = measure_ns([&] {
            results = matcher_teddy_baseline(data_sv, key_svs, teddy_data,
                                             teddy_stats);
        });
    } else {
        teddy_data = compile_teddy_data(key_svs, config);
        results =
            matcher_teddy_baseline(data_sv, key_svs, teddy_data, teddy_stats);
    }

    return results.size();
}
