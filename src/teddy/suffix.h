#pragma once

#include "findkey.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace teddy {

using Suffix = std::array<uint8_t, FINDKEY_TEDDY_MAX_COMPILED_SIGMA>;

struct SuffixSet {
    int sigma = 0;
    size_t end_quote_offset = 1;

    std::vector<Suffix> data;

    // maps original key index to suffix index
    std::vector<uint32_t> key_suffix_ids;
};

uint64_t encode_suffix(const uint8_t* suffix, int sigma) noexcept;

SuffixSet prepare_suffixes(const std::vector<std::string_view>& keys,
                           const findkey_teddy_config& config);

}  // namespace teddy
