#pragma once

#include "teddy/compile.h"
#include "teddy/grouping/builder.h"
#include "teddy/hash.h"

#include <array>
#include <utility>

namespace teddy::grouping {

template <int Sigma>
class HashGroupingBuilder final : public GroupingBuilder<Sigma> {
   public:
    HashGroupingBuilder(
        const std::vector<Suffix>& suffixes,
        findkey_teddy_compile_grouping_strategy grouping_strategy)
        : GroupingBuilder<Sigma>(suffixes, grouping_strategy) {}

    GroupedSuffixIds build() const {
        // partition
        std::array<std::vector<uint32_t>, MAX_GROUPS> buckets;
        for (uint32_t suffix_id = 0; suffix_id < this->suffixes_.size();
             ++suffix_id) {
            const uint32_t hash =
                hash_grouping_bytes(this->suffixes_[suffix_id].data(), Sigma,
                                    this->grouping_strategy_);
            buckets[hash & (MAX_GROUPS - 1)].push_back(suffix_id);
        }

        // compress into struct
        GroupedSuffixIds group_suffix_ids;
        group_suffix_ids.reserve(MAX_GROUPS);
        for (int group = 0; group < MAX_GROUPS; ++group) {
            if (buckets[group].empty()) {
                continue;
            }

            group_suffix_ids.push_back(std::move(buckets[group]));
        }
        return group_suffix_ids;
    }
};

}  // namespace teddy::grouping
