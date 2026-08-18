#pragma once

#include "teddy/grouping/score.h"

#include <array>
#include <bit>
#include <cstdint>

namespace teddy::grouping {

// Similar to PaperScore, but uses nibbles for score
template <int Sigma>
class PaperNibbleScore final {
   public:
    void add(const Suffix& suffix) noexcept {
        for (int i = 0; i < Sigma; ++i) {
            const uint8_t byte = suffix[i];
            low_nibble_unions_[i] |= byte & 0x0F;
            high_nibble_unions_[i] |= byte >> 4;
        }
        update_value();
    }

    void merge(const PaperNibbleScore& other) noexcept {
        for (int i = 0; i < Sigma; ++i) {
            low_nibble_unions_[i] |= other.low_nibble_unions_[i];
            high_nibble_unions_[i] |= other.high_nibble_unions_[i];
        }
        update_value();
    }

    [[nodiscard]] uint64_t value() const noexcept { return value_; }

   private:
    void update_value() noexcept {
        value_ = 1;
        for (int i = 0; i < Sigma; ++i) {
            value_ *=
                std::popcount(static_cast<unsigned int>(low_nibble_unions_[i]));
            value_ *= std::popcount(
                static_cast<unsigned int>(high_nibble_unions_[i]));
        }
    }

    std::array<uint8_t, Sigma> low_nibble_unions_{};
    std::array<uint8_t, Sigma> high_nibble_unions_{};
    uint64_t value_ = 0;
};

}  // namespace teddy::grouping
