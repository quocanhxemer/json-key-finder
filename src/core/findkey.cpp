#include "findkey.h"
#include "core/findkey_error.h"
#include "matchers/matcher_scalar.h"
#include "matchers/matcher_teddy_baseline.h"
#include "teddy/compile.h"
#include "teddy/verification/dispatch.h"

#if COMPILER_SUPPORTS_TEDDY
#include "matchers/matcher_teddy.h"
#endif

#include <algorithm>
#include <chrono>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

static inline bool bad_args(const uint8_t* data,
                            size_t len,
                            const uint8_t* const* keys,
                            const size_t* key_lens,
                            size_t num_keys,
                            struct findkey_result* out_results,
                            struct findkey_timing* out_timing) {
    if (!data || len == 0 || !keys || !key_lens || !out_results ||
        !out_timing || !num_keys) {
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
    if (!data || len == 0 || !keys || !key_lens || !num_keys || !teddy_stats) {
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

static int status_from_error(const FindkeyError& error) noexcept {
    switch (error.code()) {
        case FindkeyErrorCode::INVALID_ARGUMENT:
            return FINDKEY_ERR_BAD_ARGS;
        case FindkeyErrorCode::NOT_SUPPORTED:
            return FINDKEY_TEDDY_NOT_SUPPORTED;
        case FindkeyErrorCode::UNKNOWN_ALGORITHM:
            return FINDKEY_ERR_UNKNOWN_ALGO;
    }

    return FINDKEY_ERR_BAD_ARGS;
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

    if (bad_args(data, len, keys, key_lens, num_keys, out_results,
                 out_timing)) {
        if (out_status) {
            *out_status = FINDKEY_ERR_BAD_ARGS;
        }
        return 0;
    }

    const std::string_view data_sv(reinterpret_cast<const char*>(data), len);

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

    try {
        switch (algo) {
            case SCALAR: {
                out_timing->match_ns = measure_ns(
                    [&] { results = matcher_scalar(data_sv, key_svs); });
                break;
            }

            case TEDDY:
#if COMPILER_SUPPORTS_TEDDY
            {
                teddy::dispatch_verifier(
                    config.verification_strategy,
                    [&]<teddy::Verifier VerifierModel>() {
                        teddy::CompilationData teddy_data;
                        std::optional<VerifierModel> verifier;
                        out_timing->compile_ns = measure_ns([&] {
                            teddy_data = teddy::compile(key_svs, config);
                            const teddy::VerificationBuildContext context{
                                key_svs, teddy_data};
                            verifier.emplace(context);
                        });
                        out_timing->match_ns = measure_ns([&] {
                            results =
                                matcher_teddy(data_sv, teddy_data, *verifier);
                        });
                    });
                break;
            }
#else
                throw FindkeyError(FindkeyErrorCode::NOT_SUPPORTED,
                                   "Teddy is not supported by this compiler");
#endif
            case TEDDY_BASELINE: {
                teddy::dispatch_verifier(
                    config.verification_strategy,
                    [&]<teddy::Verifier VerifierModel>() {
                        teddy::CompilationData teddy_data;
                        std::optional<VerifierModel> verifier;
                        out_timing->compile_ns = measure_ns([&] {
                            teddy_data = teddy::compile(key_svs, config);
                            const teddy::VerificationBuildContext context{
                                key_svs, teddy_data};
                            verifier.emplace(context);
                        });
                        out_timing->match_ns = measure_ns([&] {
                            results = matcher_teddy_baseline(
                                data_sv, teddy_data, *verifier);
                        });
                    });
                break;
            }
            default:
                throw FindkeyError(FindkeyErrorCode::UNKNOWN_ALGORITHM,
                                   "Unknown matching algorithm");
        }

        const size_t num_positions =
            std::min(results.size(), max_out_positions);

        for (size_t i = 0; i < num_positions; ++i) {
            out_results[i] = results[i];
        }

        return results.size();
    } catch (const FindkeyError& error) {
        if (out_status) {
            *out_status = status_from_error(error);
        }
        return 0;
    }
}

extern "C" size_t findkey_with_stats(
    const uint8_t* data,
    size_t len,
    const uint8_t* const* keys,
    const size_t* key_lens,
    size_t num_keys,
    const struct findkey_teddy_config* teddy_config,
    struct findkey_teddy_stats* teddy_stats,
    int* out_status) {
    if (out_status) {
        *out_status = FINDKEY_OK;
    }
    if (bad_args_stats(data, len, keys, key_lens, num_keys, teddy_stats)) {
        if (out_status) {
            *out_status = FINDKEY_ERR_BAD_ARGS;
        }
        return 0;
    }

    *teddy_stats = {};

    const std::string_view data_sv(reinterpret_cast<const char*>(data), len);

    std::vector<std::string_view> key_svs;
    key_svs.reserve(num_keys);
    for (size_t i = 0; i < num_keys; ++i) {
        const char* k = reinterpret_cast<const char*>(keys[i]);
        key_svs.emplace_back(k, key_lens[i]);
    }
    const findkey_teddy_config default_teddy_config = FINDKEY_TEDDY_CONFIG_INIT;
    const findkey_teddy_config& config =
        teddy_config ? *teddy_config : default_teddy_config;

    try {
        const std::vector<findkey_result> results = teddy::dispatch_verifier(
            config.verification_strategy, [&]<teddy::Verifier VerifierModel>() {
                const teddy::CompilationData teddy_data =
                    teddy::compile(key_svs, config);
                const teddy::VerificationBuildContext context{key_svs,
                                                              teddy_data};
                const VerifierModel verifier(context);
                return matcher_teddy_baseline(data_sv, teddy_data, verifier,
                                              teddy_stats);
            });

        return results.size();
    } catch (const FindkeyError& error) {
        if (out_status) {
            *out_status = status_from_error(error);
        }
        return 0;
    }
}
