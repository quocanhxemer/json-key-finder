#pragma once

#include "core/key_dfa.h"
#include "findkey.h"
#include "teddy/teddy_compile.h"

#include <string_view>
#include <vector>

/*
    Acts as baseline teddy matcher without SIMD for matcher_teddy.cpp
*/

std::vector<findkey_result> matcher_teddy_baseline(
    std::string_view data,
    const TeddyCompilationData& teddy_data,
    const DFA& dfa,
    struct findkey_teddy_stats* stats = nullptr);
