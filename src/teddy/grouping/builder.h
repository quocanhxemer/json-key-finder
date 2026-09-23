#pragma once

#include "findkey.h"
#include "teddy/suffix.h"

#include <cstdint>
#include <vector>

namespace teddy::grouping {

using GroupedSuffixIds = std::vector<std::vector<uint32_t>>;

template <int Sigma>
class GroupingBuilderBase {
    static_assert(Sigma > 0 && Sigma <= FINDKEY_TEDDY_MAX_COMPILED_SIGMA,
                  "Compiled Teddy suffix byte count is out of range");

   protected:
    GroupingBuilderBase(
        const std::vector<Suffix>& suffixes,
        findkey_teddy_compile_grouping_strategy grouping_strategy)
        : suffixes_(suffixes), grouping_strategy_(grouping_strategy) {}

    ~GroupingBuilderBase() = default;

    const std::vector<Suffix>& suffixes_;
    const findkey_teddy_compile_grouping_strategy grouping_strategy_;
};

}  // namespace teddy::grouping
