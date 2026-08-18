#pragma once

#include "teddy/grouping/score.h"

#include <array>
#include <bit>
#include <cstdint>

namespace teddy::grouping {

template <int Sigma>
class PaperScore final {
   public:
    void add(const Suffix& suffix) noexcept {
        for (int i = 0; i < Sigma; ++i) {
            byte_unions_[i] |= suffix[i];
        }
        update_value();
    }

    void merge(const PaperScore& other) noexcept {
        for (int i = 0; i < Sigma; ++i) {
            byte_unions_[i] |= other.byte_unions_[i];
        }
        update_value();
    }

    [[nodiscard]] uint64_t value() const noexcept { return value_; }

   private:
    void update_value() noexcept {
        value_ = 1;
        for (uint8_t byte_union : byte_unions_) {
            value_ *= std::popcount(static_cast<unsigned int>(byte_union));
        }
    }

    std::array<uint8_t, Sigma> byte_unions_{};
    uint64_t value_ = 0;
};

static_assert(GroupingScore<PaperScore<1>>);

}  // namespace teddy::grouping
