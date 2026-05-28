#pragma once

#include "findkey.h"
#include "teddy/teddy_common.h"

#include <string_view>
#include <vector>

/*
    Acts as baseline teddy matcher without SIMD for matcher_teddy.cpp
*/

std::vector<findkey_result> matcher_teddy_baseline(
    std::string_view data,
    const std::vector<std::string_view>& keys,
    const TeddyCompilationData& teddy_data,
    struct findkey_teddy_stats* stats = nullptr);
