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

TEST(FindkeyCapabilityTest, ReportsUnsupportedBuildOrMatchesScalar) {
    constexpr std::string_view json = R"({"dummy":1})";
    const std::vector<std::string_view> keys = {"dummy"};

    switch (simd_teddy_availability()) {
        case SimdTeddyAvailability::NotCompiled: {
            const ApiRun simd = run_findkey(json, keys, TEDDY);
            EXPECT_EQ(simd.status, FINDKEY_TEDDY_NOT_SUPPORTED);
            EXPECT_EQ(simd.total, 0u);
            break;
        }
        case SimdTeddyAvailability::CpuUnsupported:
            GTEST_SKIP() << "CPU does not have SSSE3 support";
        case SimdTeddyAvailability::Available: {
            const ApiRun scalar = run_findkey(json, keys, SCALAR);
            ASSERT_TRUE(expect_success(scalar));
            const ApiRun simd = run_findkey(json, keys, TEDDY);
            expect_same_results(scalar, simd);
            break;
        }
    }
}
