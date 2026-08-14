#pragma once

#include "findkey.h"
#include "teddy/teddy_suffix.h"

#include <cstdint>
#include <vector>

std::vector<std::vector<uint32_t>> build_teddy_groups(
    const std::vector<TeddySuffix>& suffixes,
    findkey_teddy_compile_grouping_strategy grouping_strategy,
    int sigma);
