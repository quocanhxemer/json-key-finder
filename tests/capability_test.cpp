#include "utils.h"

#include <gtest/gtest.h>

#include <string_view>
#include <vector>

namespace {

using findkey_test::ApiRun;
using findkey_test::expect_same_results;
using findkey_test::expect_success;
using findkey_test::run_findkey;
using findkey_test::simd_teddy_availability;
using findkey_test::SimdTeddyAvailability;

}  // namespace

TEST(FindkeyCapabilityTest, RequiresSimdTeddyAvailability) {
    constexpr std::string_view json = R"({"dummy":1})";
    const std::vector<std::string_view> keys = {"dummy"};

    switch (simd_teddy_availability()) {
        case SimdTeddyAvailability::NotCompiled: {
            const ApiRun simd = run_findkey(json, keys, TEDDY);
            ASSERT_EQ(simd.status, FINDKEY_TEDDY_NOT_SUPPORTED);
            FAIL() << "-mssse3 is not supported by the compiler";
        }
        case SimdTeddyAvailability::CpuUnsupported:
            FAIL() << "CPU does not have SSSE3 support";
        case SimdTeddyAvailability::Available: {
            const ApiRun scalar = run_findkey(json, keys, SCALAR);
            ASSERT_TRUE(expect_success(scalar));
            const ApiRun simd = run_findkey(json, keys, TEDDY);
            expect_same_results(scalar, simd);
            break;
        }
    }
}
