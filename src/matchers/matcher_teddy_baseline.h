#pragma once

#include "findkey.h"
#include "teddy/compile.h"
#include "teddy/verification/verifier.h"

#include <string_view>
#include <vector>

/*
    Acts as baseline teddy matcher without SIMD for matcher_teddy.cpp
*/

template <teddy::Verifier VerifierModel>
std::vector<findkey_result> matcher_teddy_baseline(
    std::string_view data,
    const teddy::CompilationData& teddy_data,
    const VerifierModel& verifier,
    struct findkey_teddy_stats* stats = nullptr);
