#pragma once

#include "teddy/compile.h"
#include "teddy/grouping/builder.h"

#include <array>
#include <limits>
#include <utility>

namespace teddy::grouping {

template <int Sigma>
class GreedyGroupingBuilder final : public GroupingBuilder<Sigma> {
   public:
    GreedyGroupingBuilder(
        const std::vector<Suffix>& suffixes,
        findkey_teddy_compile_grouping_strategy grouping_strategy)
        : GroupingBuilder<Sigma>(suffixes, grouping_strategy) {}

    GroupedSuffixIds build() const {
        std::vector<SuffixGroup> groups;
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
            group_suffix_ids.push_back(std::move(group.suffix_ids));
        }
        return group_suffix_ids;
    }

   private:
    struct SuffixGroup {
        std::vector<uint32_t> suffix_ids;
        std::array<uint8_t, Sigma> b{};

        uint32_t score = 1;

        SuffixGroup(uint32_t suffix_id, const Suffix& suffix) {
            suffix_ids.push_back(suffix_id);
            for (int i = 0; i < Sigma; ++i) {
                b[i] = suffix[i];
                score *= __builtin_popcount(b[i]);
            }
        }

        static uint32_t merge_score(const SuffixGroup& a,
                                    const SuffixGroup& b) {
            uint32_t s = 1;
            for (int i = 0; i < Sigma; ++i) {
                const uint8_t merged = a.b[i] | b.b[i];
                s *= __builtin_popcount(merged);
            }
            return s;
        }

        void merge_from(const SuffixGroup& source, uint32_t merged_score) {
            for (int i = 0; i < Sigma; ++i) {
                b[i] |= source.b[i];
            }

            suffix_ids.reserve(suffix_ids.size() + source.suffix_ids.size());
            suffix_ids.insert(suffix_ids.end(), source.suffix_ids.begin(),
                              source.suffix_ids.end());
            score = merged_score;
        }
    };

    bool merge_best_pair(std::vector<SuffixGroup>& groups) const {
        int best = std::numeric_limits<int>::max();
        size_t best_i = groups.size();
        size_t best_j = groups.size();
        uint32_t selected_merge_score = 0;

        for (size_t i = 0; i < groups.size(); ++i) {
            for (size_t j = i + 1; j < groups.size(); ++j) {
                // new_score - old_score might be negative
                // hence casting them to int
                int new_score = static_cast<int>(
                    SuffixGroup::merge_score(groups[i], groups[j]));
                int old_score = static_cast<int>(groups[i].score) +
                                static_cast<int>(groups[j].score);

                if (this->grouping_strategy_ == TEDDY_COMPILE_PAPER_GREEDY &&
                    new_score < old_score) {
                    best_i = i;
                    best_j = j;
                    selected_merge_score = static_cast<uint32_t>(new_score);
                    break;
                }

                if (best > new_score - old_score) {
                    best = new_score - old_score;
                    best_i = i;
                    best_j = j;
                    selected_merge_score = static_cast<uint32_t>(new_score);
                }
            }
        }

        // shouldn't happen
        // should always find a pair to merge
        if (best_i == groups.size()) {
            return false;
        }

        SuffixGroup& target = groups[best_i];
        SuffixGroup& source = groups[best_j];
        target.merge_from(source, selected_merge_score);

        groups.erase(groups.begin() + best_j);
        return true;
    }
};

}  // namespace teddy::grouping
