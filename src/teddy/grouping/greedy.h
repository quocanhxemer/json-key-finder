#pragma once

#include "teddy/compile.h"
#include "teddy/grouping/builder.h"
#include "teddy/grouping/group.h"
#include "teddy/grouping/score.h"

#include <cstdint>
#include <limits>
#include <utility>

namespace teddy::grouping {

enum class GreedySelectionPolicy {
    Paper,
    MinDelta,
};

template <int Sigma,
          GroupingScore ScoreModel,
          GreedySelectionPolicy SelectionPolicy>
class GreedyGroupingBuilder final : public GroupingBuilder<Sigma> {
    using Group = SuffixGroup<ScoreModel>;

   public:
    explicit GreedyGroupingBuilder(const std::vector<Suffix>& suffixes)
        : GroupingBuilder<Sigma>(suffixes, strategy()) {}

    GroupedSuffixIds build() const {
        std::vector<Group> groups;
        groups.reserve(this->suffixes_.size());
        for (uint32_t i = 0; i < this->suffixes_.size(); ++i) {
            groups.emplace_back(i, this->suffixes_[i]);
        }

        while (groups.size() > MAX_GROUPS) {
            if (!merge_best_pair(groups)) {
                break;
            }
        }

        GroupedSuffixIds group_suffix_ids;
        group_suffix_ids.reserve(groups.size());
        for (auto& group : groups) {
            group_suffix_ids.push_back(std::move(group).take_suffix_ids());
        }
        return group_suffix_ids;
    }

   private:
    bool merge_best_pair(std::vector<Group>& groups) const {
        int64_t best = std::numeric_limits<int64_t>::max();
        size_t best_i = groups.size();
        size_t best_j = groups.size();

        for (size_t i = 0; i < groups.size(); ++i) {
            for (size_t j = i + 1; j < groups.size(); ++j) {
                const uint64_t new_score =
                    groups[i].score_if_merged_with(groups[j]);
                const uint64_t old_score =
                    groups[i].score() + groups[j].score();

                // new_score - old_score might be negative
                // hence casting them to int
                const int64_t score_delta = static_cast<int64_t>(new_score) -
                                            static_cast<int64_t>(old_score);

                if constexpr (SelectionPolicy == GreedySelectionPolicy::Paper) {
                    if (new_score < old_score) {
                        best_i = i;
                        best_j = j;
                        break;
                    }
                }

                if (best > score_delta) {
                    best = score_delta;
                    best_i = i;
                    best_j = j;
                }
            }
        }

        // shouldn't happen
        // should always find a pair to merge
        if (best_i == groups.size()) {
            return false;
        }

        Group& target = groups[best_i];
        Group& source = groups[best_j];
        target.absorb(std::move(source));

        groups.erase(groups.begin() + best_j);
        return true;
    }

    static constexpr findkey_teddy_compile_grouping_strategy strategy() {
        if constexpr (SelectionPolicy == GreedySelectionPolicy::Paper) {
            return TEDDY_COMPILE_GREEDY_PAPER_POLICY;
        }
        return TEDDY_COMPILE_GREEDY_MIN_DELTA;
    }
};

}  // namespace teddy::grouping
