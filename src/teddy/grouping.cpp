#include "teddy/grouping.h"

#include "core/findkey_error.h"
#include "teddy/dispatch.h"
#include "teddy/grouping/builder.h"
#include "teddy/grouping/greedy.h"
#include "teddy/grouping/hash.h"
#include "teddy/grouping/sorted.h"

namespace teddy {

std::vector<std::vector<uint32_t>> build_groups(
    const std::vector<Suffix>& suffixes,
    findkey_teddy_compile_grouping_strategy grouping_strategy,
    int sigma) {
    return dispatch_sigma<grouping::GroupedSuffixIds>(sigma, [&]<int Sigma>() {
        switch (grouping_strategy) {
            case TEDDY_COMPILE_PAPER_GREEDY:
            case TEDDY_COMPILE_PAPER_IMPROVED_GREEDY:
                return grouping::GreedyGroupingBuilder<Sigma>(suffixes,
                                                              grouping_strategy)
                    .build();
            case TEDDY_COMPILE_HASH_STD:
            case TEDDY_COMPILE_HASH_ADLER32:
            case TEDDY_COMPILE_HASH_CRC32:
            case TEDDY_COMPILE_HASH_XXHASH:
            case TEDDY_COMPILE_HASH_FNV1A:
                return grouping::HashGroupingBuilder<Sigma>(suffixes,
                                                            grouping_strategy)
                    .build();
            case TEDDY_COMPILE_SORTED_SUFFIX_ROUND_ROBIN:
            case TEDDY_COMPILE_SORTED_SUFFIX_PARTITION:
                return grouping::SortedGroupingBuilder<Sigma>(suffixes,
                                                              grouping_strategy)
                    .build();
            default:
                throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                                   "Unknown Teddy grouping strategy");
        }
    });
}

}  // namespace teddy
