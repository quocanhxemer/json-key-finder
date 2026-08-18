#include "teddy/grouping.h"

#include "core/findkey_error.h"
#include "teddy/dispatch.h"
#include "teddy/grouping/builder.h"
#include "teddy/grouping/greedy.h"
#include "teddy/grouping/hash.h"
#include "teddy/grouping/scores/dispatch.h"
#include "teddy/grouping/sorted.h"

namespace teddy {

std::vector<std::vector<uint32_t>> build_groups(
    const std::vector<Suffix>& suffixes,
    findkey_teddy_grouping_config grouping_config,
    int sigma) {
    return dispatch_sigma(sigma, [&]<int Sigma>() {
        switch (grouping_config.strategy) {
            case TEDDY_COMPILE_GREEDY_PAPER_POLICY:
                return grouping::dispatch_grouping_score<Sigma>(
                    grouping_config.score,
                    [&]<grouping::GroupingScore ScoreModel>() {
                        return grouping::GreedyGroupingBuilder<
                                   Sigma, ScoreModel,
                                   grouping::GreedySelectionPolicy::Paper>(
                                   suffixes)
                            .build();
                    });
            case TEDDY_COMPILE_GREEDY_MIN_DELTA:
                return grouping::dispatch_grouping_score<Sigma>(
                    grouping_config.score,
                    [&]<grouping::GroupingScore ScoreModel>() {
                        return grouping::GreedyGroupingBuilder<
                                   Sigma, ScoreModel,
                                   grouping::GreedySelectionPolicy::MinDelta>(
                                   suffixes)
                            .build();
                    });
            case TEDDY_COMPILE_HASH_STD:
            case TEDDY_COMPILE_HASH_ADLER32:
            case TEDDY_COMPILE_HASH_CRC32:
            case TEDDY_COMPILE_HASH_XXHASH:
            case TEDDY_COMPILE_HASH_FNV1A:
                return grouping::HashGroupingBuilder<Sigma>(
                           suffixes, grouping_config.strategy)
                    .build();
            case TEDDY_COMPILE_SORTED_SUFFIX_ROUND_ROBIN:
            case TEDDY_COMPILE_SORTED_SUFFIX_PARTITION:
                return grouping::SortedGroupingBuilder<Sigma>(
                           suffixes, grouping_config.strategy)
                    .build();
            default:
                throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                                   "Unknown Teddy grouping strategy");
        }
    });
}

}  // namespace teddy
