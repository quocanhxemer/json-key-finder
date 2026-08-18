#pragma once

#include "core/findkey_error.h"
#include "findkey.h"
#include "teddy/grouping/scores/nibble_count.h"
#include "teddy/grouping/scores/paper.h"
#include "teddy/grouping/scores/paper_nibble.h"

#include <utility>

namespace teddy::grouping {

template <int Sigma, typename Function>
decltype(auto) dispatch_grouping_score(findkey_teddy_grouping_score score,
                                       Function&& function) {
    switch (score) {
        case TEDDY_GROUPING_SCORE_PAPER:
            return std::forward<Function>(function)
                .template operator()<PaperScore<Sigma>>();
        case TEDDY_GROUPING_SCORE_PAPER_NIBBLE:
            return std::forward<Function>(function)
                .template operator()<PaperNibbleScore<Sigma>>();
        case TEDDY_GROUPING_SCORE_NIBBLE_COUNT:
            return std::forward<Function>(function)
                .template operator()<NibbleCountScore<Sigma>>();
        default:
            throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                               "Unknown Teddy grouping score");
    }
}

}  // namespace teddy::grouping
