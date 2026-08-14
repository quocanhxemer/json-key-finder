#pragma once

#include "findkey.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

using TeddySuffix = std::array<uint8_t, FINDKEY_TEDDY_MAX_SIGMA>;

struct TeddySuffixSet {
    int sigma = 0;
    size_t end_quote_offset = 1;

    std::vector<TeddySuffix> data;
};

TeddySuffixSet prepare_teddy_suffixes(const std::vector<std::string_view>& keys,
                                      const findkey_teddy_config& config);
