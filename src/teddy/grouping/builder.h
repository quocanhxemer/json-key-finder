#pragma once

#include "findkey.h"
#include "teddy/suffix.h"

#include <cstdint>
#include <vector>

namespace teddy::grouping {

using GroupedSuffixIds = std::vector<std::vector<uint32_t>>;

template <int Sigma>
class GroupingBuilder {
    static_assert(Sigma > 0 && Sigma <= FINDKEY_TEDDY_MAX_SIGMA,
                  "Teddy sigma is out of range");

   protected:
    GroupingBuilder(const std::vector<Suffix>& suffixes,
                    findkey_teddy_compile_grouping_strategy grouping_strategy)
        : suffixes_(suffixes), grouping_strategy_(grouping_strategy) {}

    ~GroupingBuilder() = default;

    const std::vector<Suffix>& suffixes_;
    const findkey_teddy_compile_grouping_strategy grouping_strategy_;
};

}  // namespace teddy::grouping
