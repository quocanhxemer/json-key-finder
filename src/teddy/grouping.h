#pragma once

#include "findkey.h"
#include "teddy/suffix.h"

#include <cstdint>
#include <vector>

namespace teddy {

std::vector<std::vector<uint32_t>> build_groups(
    const std::vector<Suffix>& suffixes,
    findkey_teddy_grouping_config grouping_config,
    int sigma);

}  // namespace teddy
