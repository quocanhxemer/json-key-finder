#include "teddy/grouping.h"

#include "core/findkey_error.h"
#include "teddy/compile.h"
#include "teddy/dispatch.h"
#include "teddy/hash.h"

#include <algorithm>
#include <array>
#include <climits>
#include <numeric>
#include <utility>

namespace teddy {

template <int Sigma>
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

    static uint32_t merge_score(const SuffixGroup& a, const SuffixGroup& b) {
        uint32_t s = 1;
        for (int i = 0; i < Sigma; ++i) {
            uint8_t merged = a.b[i] | b.b[i];
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

template <int Sigma>
class GroupingBuilder {
   public:
    GroupingBuilder(const std::vector<Suffix>& suffixes,
                    findkey_teddy_compile_grouping_strategy grouping_strategy)
        : suffixes_(suffixes), grouping_strategy_(grouping_strategy) {}

    std::vector<std::vector<uint32_t>> build() const {
        switch (grouping_strategy_) {
            case TEDDY_COMPILE_PAPER_GREEDY:
                return build_greedy_groups(true);
            case TEDDY_COMPILE_PAPER_IMPROVED_GREEDY:
                return build_greedy_groups(false);
            case TEDDY_COMPILE_HASH_STD:
            case TEDDY_COMPILE_HASH_ADLER32:
            case TEDDY_COMPILE_HASH_CRC32:
            case TEDDY_COMPILE_HASH_XXHASH:
            case TEDDY_COMPILE_HASH_FNV1A:
                return build_hash_groups();
            case TEDDY_COMPILE_SORTED_SUFFIX_ROUND_ROBIN:
                return build_sorted_suffix_round_robin_groups();
            case TEDDY_COMPILE_SORTED_SUFFIX_PARTITION:
                return build_sorted_suffix_partition_groups();
            default:
                throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                                   "Unknown Teddy grouping strategy");
        }
    }

   private:
    using Group = SuffixGroup<Sigma>;

    std::vector<std::vector<uint32_t>> build_greedy_groups(
        bool paper_early_exit) const {
        std::vector<Group> groups;
        groups.reserve(suffixes_.size());
        for (uint32_t i = 0; i < suffixes_.size(); ++i) {
            groups.emplace_back(i, suffixes_[i]);
        }

        while (groups.size() > MAX_GROUPS) {
            // find best pair to merge
            int best = INT_MAX;
            int best_i = -1;
            int best_j = -1;
            uint32_t selected_merge_score = 0;

            for (size_t i = 0; i < groups.size(); ++i) {
                for (size_t j = i + 1; j < groups.size(); ++j) {
                    int new_score = Group::merge_score(groups[i], groups[j]);
                    int old_score = static_cast<int>(groups[i].score) +
                                    static_cast<int>(groups[j].score);
                    if (paper_early_exit && new_score < old_score) {
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
            // an iteration should always find a pair to merge
            if (best_i == -1) {
                break;
            }

            Group& target = groups[best_i];
            Group& source = groups[best_j];
            target.merge_from(source, selected_merge_score);

            groups.erase(groups.begin() + best_j);
        }

        std::vector<std::vector<uint32_t>> group_suffix_ids;
        group_suffix_ids.reserve(groups.size());
        for (auto& group : groups) {
            group_suffix_ids.push_back(std::move(group.suffix_ids));
        }
        return group_suffix_ids;
    }

    std::vector<std::vector<uint32_t>> build_hash_groups() const {
        // partition
        std::array<std::vector<uint32_t>, MAX_GROUPS> buckets;
        for (uint32_t suffix_id = 0; suffix_id < suffixes_.size();
             ++suffix_id) {
            const uint32_t hash = hash_grouping_bytes(
                suffixes_[suffix_id].data(), Sigma, grouping_strategy_);

            buckets[hash & (MAX_GROUPS - 1)].push_back(suffix_id);
        }

        // compress into struct
        std::vector<std::vector<uint32_t>> group_suffix_ids;
        group_suffix_ids.reserve(MAX_GROUPS);
        for (int group = 0; group < MAX_GROUPS; ++group) {
            if (buckets[group].empty()) {
                continue;
            }

            group_suffix_ids.push_back(std::move(buckets[group]));
        }
        return group_suffix_ids;
    }

    std::vector<uint32_t> sorted_suffix_ids() const {
        std::vector<uint32_t> suffix_ids(suffixes_.size());
        std::iota(suffix_ids.begin(), suffix_ids.end(), uint32_t{0});

        std::sort(suffix_ids.begin(), suffix_ids.end(),
                  [&](uint32_t left_suffix_id, uint32_t right_suffix_id) {
                      for (int suffix_index = 0; suffix_index < Sigma;
                           ++suffix_index) {
                          const uint8_t left_byte =
                              suffixes_[left_suffix_id][suffix_index];
                          const uint8_t right_byte =
                              suffixes_[right_suffix_id][suffix_index];
                          if (left_byte != right_byte) {
                              return left_byte < right_byte;
                          }
                      }
                      return left_suffix_id < right_suffix_id;
                  });

        return suffix_ids;
    }

    std::vector<std::vector<uint32_t>> build_sorted_suffix_round_robin_groups()
        const {
        const std::vector<uint32_t> suffix_ids = sorted_suffix_ids();
        const size_t num_groups =
            std::min(suffix_ids.size(), static_cast<size_t>(MAX_GROUPS));

        std::vector<std::vector<uint32_t>> group_suffix_ids(num_groups);
        for (size_t suffix_index = 0; suffix_index < suffix_ids.size();
             ++suffix_index) {
            group_suffix_ids[suffix_index % num_groups].push_back(
                suffix_ids[suffix_index]);
        }
        return group_suffix_ids;
    }

    std::vector<std::vector<uint32_t>> build_sorted_suffix_partition_groups()
        const {
        const std::vector<uint32_t> suffix_ids = sorted_suffix_ids();
        const size_t num_groups =
            std::min(suffix_ids.size(), static_cast<size_t>(MAX_GROUPS));

        std::vector<std::vector<uint32_t>> group_suffix_ids;
        group_suffix_ids.reserve(num_groups);
        for (size_t group_index = 0; group_index < num_groups; ++group_index) {
            const size_t begin_index =
                group_index * suffix_ids.size() / num_groups;
            const size_t end_index =
                (group_index + 1) * suffix_ids.size() / num_groups;

            std::vector<uint32_t> group;
            group.reserve(end_index - begin_index);
            for (size_t suffix_index = begin_index; suffix_index < end_index;
                 ++suffix_index) {
                group.push_back(suffix_ids[suffix_index]);
            }
            group_suffix_ids.push_back(std::move(group));
        }
        return group_suffix_ids;
    }

    const std::vector<Suffix>& suffixes_;
    findkey_teddy_compile_grouping_strategy grouping_strategy_;
};

std::vector<std::vector<uint32_t>> build_groups(
    const std::vector<Suffix>& suffixes,
    findkey_teddy_compile_grouping_strategy grouping_strategy,
    int sigma) {
    return dispatch_sigma<std::vector<std::vector<uint32_t>>>(
        sigma, [&]<int Sigma>() {
            return GroupingBuilder<Sigma>(suffixes, grouping_strategy).build();
        });
}

}  // namespace teddy
