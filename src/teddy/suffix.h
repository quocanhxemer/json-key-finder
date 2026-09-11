#pragma once

#include "findkey.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace teddy {

using Suffix = std::array<uint8_t, FINDKEY_TEDDY_MAX_SIGMA>;

struct SuffixSet {
    int sigma = 0;
    size_t end_quote_offset = 1;

    std::vector<Suffix> data;

    // maps original key index to suffix index
    std::vector<uint32_t> key_suffix_ids;
};

SuffixSet prepare_suffixes(const std::vector<std::string_view>& keys,
                           const findkey_teddy_config& config);

}  // namespace teddy
