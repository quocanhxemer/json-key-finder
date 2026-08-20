#pragma once

#include "findkey.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace findkey_test {

struct ApiRun {
    int status = FINDKEY_ERR_BAD_ARGS;
    size_t total = 0;
    std::vector<findkey_result> results;
};

enum class SimdTeddyAvailability {
    Available,
    NotCompiled,
    CpuUnsupported,
};

std::string load_json_fixture(std::string_view filename);

ApiRun run_findkey(std::string_view json,
                   const std::vector<std::string_view>& keys,
                   findkey_algo algorithm,
                   const findkey_teddy_config* teddy_config = nullptr);

bool expect_success(const ApiRun& run);

void expect_same_results(const ApiRun& expected, const ApiRun& actual);

void expect_teddy_matchers_match(
    const ApiRun& expected,
    std::string_view json,
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config* teddy_config = nullptr);

void expect_teddy_matches_scalar(std::string_view json,
                                 const std::vector<std::string_view>& keys,
                                 const findkey_teddy_config& config);

SimdTeddyAvailability simd_teddy_availability() noexcept;

}  // namespace findkey_test
