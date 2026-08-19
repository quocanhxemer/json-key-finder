#pragma once

#include "core/findkey_error.h"
#include "teddy/compile.h"
#include "teddy/grouping/builder.h"
#include "teddy/grouping/score.h"

#include <algorithm>
#include <limits>
#include <numeric>

namespace teddy::grouping {

namespace detail {

template <int Sigma>
std::vector<uint32_t> sorted_suffix_ids(const std::vector<Suffix>& suffixes) {
    std::vector<uint32_t> suffix_ids(suffixes.size());
    std::iota(suffix_ids.begin(), suffix_ids.end(), uint32_t{0});

    std::sort(suffix_ids.begin(), suffix_ids.end(),
              [&](uint32_t left_suffix_id, uint32_t right_suffix_id) {
                  for (int i = 0; i < Sigma; ++i) {
                      const uint8_t left_byte = suffixes[left_suffix_id][i];
                      const uint8_t right_byte = suffixes[right_suffix_id][i];
                      if (left_byte != right_byte) {
                          return left_byte < right_byte;
                      }
                  }
                  return left_suffix_id < right_suffix_id;
              });

    return suffix_ids;
}

}  // namespace detail

template <int Sigma>
class SortedGroupingBuilder final : public GroupingBuilder<Sigma> {
   public:
    SortedGroupingBuilder(
        const std::vector<Suffix>& suffixes,
        findkey_teddy_compile_grouping_strategy grouping_strategy)
        : GroupingBuilder<Sigma>(suffixes, grouping_strategy) {}

    GroupedSuffixIds build() const {
        const std::vector<uint32_t> suffix_ids =
            detail::sorted_suffix_ids<Sigma>(this->suffixes_);
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

template <int Sigma, GroupingScore ScoreModel>
class SortedOptimalGroupingBuilder final : public GroupingBuilder<Sigma> {
   public:
    explicit SortedOptimalGroupingBuilder(const std::vector<Suffix>& suffixes)
        : GroupingBuilder<Sigma>(
              suffixes,
              TEDDY_COMPILE_SORTED_SUFFIX_OPTIMAL_PARTITION) {}

    GroupedSuffixIds build() const {
        const std::vector<uint32_t> suffix_ids =
            detail::sorted_suffix_ids<Sigma>(this->suffixes_);
        const size_t num_groups =
            std::min(suffix_ids.size(), static_cast<size_t>(MAX_GROUPS));

        struct PartitionState {
            uint64_t score = std::numeric_limits<uint64_t>::max();
            size_t largest_group = std::numeric_limits<size_t>::max();
            size_t previous_boundary = std::numeric_limits<size_t>::max();
        };

        // dp[g][s] = best partitioning the first s suffixes into g groups
        std::vector<std::vector<PartitionState>> dp(
            num_groups + 1, std::vector<PartitionState>(suffix_ids.size() + 1));

        dp[0][0] = {
            .score = 0,
            .largest_group = 0,
            .previous_boundary = 0,
        };

        // loop through all partitions [begin, end)
        for (size_t begin = 0; begin < suffix_ids.size(); ++begin) {
            ScoreModel partition_score;
            for (size_t end = begin + 1; end <= suffix_ids.size(); ++end) {
                partition_score.add(this->suffixes_[suffix_ids[end - 1]]);
                const uint64_t group_score = partition_score.value();
                const size_t max_group_count = std::min(num_groups, begin + 1);

                for (size_t g = 1; g <= max_group_count; ++g) {
                    const PartitionState& prev = dp[g - 1][begin];

                    if (prev.score == std::numeric_limits<uint64_t>::max()) {
                        continue;
                    }

                    // overflow
                    if (group_score >
                        std::numeric_limits<uint64_t>::max() - prev.score) {
                        continue;
                    }

                    const uint64_t candidate_score = prev.score + group_score;
                    const size_t candidate_largest_group =
                        std::max(prev.largest_group, end - begin);
                    PartitionState& current = dp[g][end];

                    if (candidate_score < current.score ||
                        (candidate_score == current.score &&
                         candidate_largest_group < current.largest_group)) {
                        current = {
                            .score = candidate_score,
                            .largest_group = candidate_largest_group,
                            .previous_boundary = begin,
                        };
                    }
                }
            }
        }

        GroupedSuffixIds group_suffix_ids(num_groups);
        size_t end = suffix_ids.size();
        for (size_t group_count = num_groups; group_count > 0; --group_count) {
            const size_t begin = dp[group_count][end].previous_boundary;
            if (begin >= end) {
                throw FindkeyError(
                    FindkeyErrorCode::INVALID_ARGUMENT,
                    "Failed to optimally partition Teddy suffixes");
            }

            group_suffix_ids[group_count - 1].assign(suffix_ids.begin() + begin,
                                                     suffix_ids.begin() + end);
            end = begin;
        }

        return group_suffix_ids;
    }
};

}  // namespace teddy::grouping
