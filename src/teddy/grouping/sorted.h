#pragma once

#include "core/findkey_error.h"
#include "teddy/compile.h"
#include "teddy/grouping/builder.h"

#include <algorithm>
#include <numeric>

namespace teddy::grouping {

template <int Sigma>
class SortedGroupingBuilder final : public GroupingBuilder<Sigma> {
   public:
    SortedGroupingBuilder(
        const std::vector<Suffix>& suffixes,
        findkey_teddy_compile_grouping_strategy grouping_strategy)
        : GroupingBuilder<Sigma>(suffixes, grouping_strategy) {}

    GroupedSuffixIds build() const {
        const std::vector<uint32_t> suffix_ids = sorted_suffix_ids();
        const size_t num_groups =
            std::min(suffix_ids.size(), static_cast<size_t>(MAX_GROUPS));

        switch (this->grouping_strategy_) {
            case TEDDY_COMPILE_SORTED_SUFFIX_ROUND_ROBIN:
                return build_sorted_suffix_round_robin_groups(suffix_ids,
                                                              num_groups);
            case TEDDY_COMPILE_SORTED_SUFFIX_PARTITION:
                return build_sorted_suffix_partition_groups(suffix_ids,
                                                            num_groups);
            default:
                throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                                   "Invalid Teddy sorted grouping strategy");
        }
    }

   private:
    std::vector<uint32_t> sorted_suffix_ids() const {
        std::vector<uint32_t> suffix_ids(this->suffixes_.size());
        std::iota(suffix_ids.begin(), suffix_ids.end(), uint32_t{0});

        std::sort(suffix_ids.begin(), suffix_ids.end(),
                  [&](uint32_t left_suffix_id, uint32_t right_suffix_id) {
                      for (int i = 0; i < Sigma; ++i) {
                          const uint8_t left_byte =
                              this->suffixes_[left_suffix_id][i];
                          const uint8_t right_byte =
                              this->suffixes_[right_suffix_id][i];
                          if (left_byte != right_byte) {
                              return left_byte < right_byte;
                          }
                      }
                      return left_suffix_id < right_suffix_id;
                  });

        return suffix_ids;
    }

    static GroupedSuffixIds build_sorted_suffix_round_robin_groups(
        const std::vector<uint32_t>& suffix_ids,
        size_t num_groups) {
        GroupedSuffixIds group_suffix_ids(num_groups);
        for (size_t i = 0; i < suffix_ids.size(); ++i) {
            group_suffix_ids[i % num_groups].push_back(suffix_ids[i]);
        }
        return group_suffix_ids;
    }

    static GroupedSuffixIds build_sorted_suffix_partition_groups(
        const std::vector<uint32_t>& suffix_ids,
        size_t num_groups) {
        GroupedSuffixIds group_suffix_ids;
        group_suffix_ids.reserve(num_groups);
        for (size_t group_index = 0; group_index < num_groups; ++group_index) {
            const size_t begin_index =
                group_index * suffix_ids.size() / num_groups;
            const size_t end_index =
                (group_index + 1) * suffix_ids.size() / num_groups;

            std::vector<uint32_t> group;
            group.reserve(end_index - begin_index);
            for (size_t i = begin_index; i < end_index; ++i) {
                group.push_back(suffix_ids[i]);
            }
            group_suffix_ids.push_back(std::move(group));
        }
        return group_suffix_ids;
    }
};

}  // namespace teddy::grouping
