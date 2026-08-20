#include "utils.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

#ifndef FINDKEY_TEST_DATA_DIR
#error "FINDKEY_TEST_DATA_DIR must identify the test fixture directory"
#endif

#ifndef COMPILER_SUPPORTS_TEDDY
#define COMPILER_SUPPORTS_TEDDY 0
#endif

namespace findkey_test {

std::string load_json_fixture(std::string_view filename) {
    const std::filesystem::path path =
        std::filesystem::path(FINDKEY_TEST_DATA_DIR) / std::string(filename);
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Failed to open JSON test fixture: " +
                                 path.string());
    }

    std::string contents{std::istreambuf_iterator<char>(input),
                         std::istreambuf_iterator<char>()};
    if (input.bad()) {
        throw std::runtime_error("Failed to read JSON test fixture: " +
                                 path.string());
    }
    return contents;
}

ApiRun run_findkey(std::string_view json,
                   const std::vector<std::string_view>& keys,
                   findkey_algo algorithm,
                   const findkey_teddy_config* teddy_config) {
    std::vector<const uint8_t*> key_data;
    std::vector<size_t> key_lengths;
    key_data.reserve(keys.size());
    key_lengths.reserve(keys.size());

    for (std::string_view key : keys) {
        key_data.push_back(reinterpret_cast<const uint8_t*>(key.data()));
        key_lengths.push_back(key.size());
    }

    std::vector<findkey_result> output(std::max<size_t>(json.size(), 1));
    findkey_timing timing{};
    ApiRun run;

    run.total = findkey(reinterpret_cast<const uint8_t*>(json.data()),
                        json.size(), key_data.data(), key_lengths.data(),
                        keys.size(), algorithm, teddy_config, output.data(),
                        output.size(), &run.status, &timing);

    output.resize(std::min(run.total, output.size()));
    run.results = std::move(output);
    return run;
}

bool expect_success(const ApiRun& run) {
    const bool successful = run.status == FINDKEY_OK;
    const bool retained_all_results = run.results.size() == run.total;
    EXPECT_EQ(run.status, FINDKEY_OK);
    EXPECT_EQ(run.results.size(), run.total);
    return successful && retained_all_results;
}

void expect_same_results(const ApiRun& expected, const ApiRun& actual) {
    if (!expect_success(expected) || !expect_success(actual)) {
        return;
    }

    EXPECT_EQ(actual.total, expected.total);

    if (actual.total != expected.total) {
        return;
    }

    for (size_t i = 0; i < expected.results.size(); ++i) {
        SCOPED_TRACE(::testing::Message() << "result index " << i);
        EXPECT_EQ(actual.results[i].position, expected.results[i].position);
        EXPECT_EQ(actual.results[i].key_id, expected.results[i].key_id);
    }
}

void expect_teddy_matchers_match(const ApiRun& expected,
                                 std::string_view json,
                                 const std::vector<std::string_view>& keys,
                                 const findkey_teddy_config* teddy_config) {
    {
        SCOPED_TRACE("algorithm: TEDDY_BASELINE");
        const ApiRun baseline =
            run_findkey(json, keys, TEDDY_BASELINE, teddy_config);
        expect_same_results(expected, baseline);
    }

    if (simd_teddy_availability() == SimdTeddyAvailability::Available) {
        SCOPED_TRACE("algorithm: TEDDY");
        const ApiRun simd = run_findkey(json, keys, TEDDY, teddy_config);
        expect_same_results(expected, simd);
    }
}

void expect_teddy_matches_scalar(std::string_view json,
                                 const std::vector<std::string_view>& keys,
                                 const findkey_teddy_config& config) {
    const ApiRun scalar = run_findkey(json, keys, SCALAR);
    if (!expect_success(scalar)) {
        return;
    }
    expect_teddy_matchers_match(scalar, json, keys, &config);
}

SimdTeddyAvailability simd_teddy_availability() noexcept {
#if !COMPILER_SUPPORTS_TEDDY
    return SimdTeddyAvailability::NotCompiled;
#elif (defined(__GNUC__) || defined(__clang__)) && \
    (defined(__i386__) || defined(__x86_64__))
    __builtin_cpu_init();
    return __builtin_cpu_supports("ssse3")
               ? SimdTeddyAvailability::Available
               : SimdTeddyAvailability::CpuUnsupported;
#else
    return SimdTeddyAvailability::Available;
#endif
}

}  // namespace findkey_test
