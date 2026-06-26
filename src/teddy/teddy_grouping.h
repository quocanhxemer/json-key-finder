#pragma once

#include "findkey.h"

#include <cstdint>
#include <string_view>
#include <vector>

std::vector<std::vector<uint32_t>> build_teddy_groups(
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config,
    int sigma);
