#pragma once

#include "findkey.h"
#include "teddy/suffix.h"

#include <cstdint>
#include <vector>

namespace teddy {

std::vector<std::vector<uint32_t>> build_groups(
    const std::vector<Suffix>& suffixes,
    findkey_teddy_compile_grouping_strategy grouping_strategy,
    int sigma);

}  // namespace teddy
