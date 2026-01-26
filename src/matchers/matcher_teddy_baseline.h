#pragma once

#include "findkey.h"

#include <string_view>
#include <vector>

/*
    Acts as baseline teddy matcher without SIMD for matcher_teddy.cpp
*/
std::vector<findkey_result> matcher_teddy_baseline(
    std::string_view data,
    const std::vector<std::string_view>& keys);
