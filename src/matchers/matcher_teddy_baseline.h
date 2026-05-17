#pragma once

#include "findkey.h"

#include <string_view>
#include <vector>

/*
    Acts as baseline teddy matcher without SIMD for matcher_teddy.cpp
*/
std::vector<findkey_result> matcher_teddy_baseline(
    std::string_view data,
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config);

std::vector<findkey_result> matcher_teddy_baseline_collect_stats(
    std::string_view data,
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config,
    struct findkey_teddy_stats* stats);
