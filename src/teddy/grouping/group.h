#pragma once

#include "teddy/grouping/score.h"
#include "teddy/suffix.h"

#include <cstdint>
#include <iterator>
#include <utility>
#include <vector>

namespace teddy::grouping {

template <GroupingScore ScoreModel>
class SuffixGroup final {
   public:
    SuffixGroup(uint32_t suffix_id, const Suffix& suffix)
        : suffix_ids_{suffix_id} {
        score_.add(suffix);
    }

    [[nodiscard]] uint64_t score() const noexcept { return score_.value(); }

    [[nodiscard]] uint64_t score_if_merged_with(
        const SuffixGroup& other) const noexcept {
        ScoreModel merged = score_;
        merged.merge(other.score_);
        return merged.value();
    }

    void absorb(SuffixGroup&& source) {
        suffix_ids_.reserve(suffix_ids_.size() + source.suffix_ids_.size());
        score_.merge(source.score_);
        suffix_ids_.insert(suffix_ids_.end(),
                           std::make_move_iterator(source.suffix_ids_.begin()),
                           std::make_move_iterator(source.suffix_ids_.end()));
    }

    [[nodiscard]] std::vector<uint32_t> take_suffix_ids() && noexcept {
        return std::move(suffix_ids_);
    }

   private:
    std::vector<uint32_t> suffix_ids_;
    ScoreModel score_;
};

}  // namespace teddy::grouping
