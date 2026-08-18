#pragma once

#include "teddy/grouping/score.h"

#include <array>
#include <bit>
#include <cstdint>

namespace teddy::grouping {

// Count the number of unique nibbles in each byte of the suffixes
// then take their product as the score
template <int Sigma>
class NibbleCountScore final {
   public:
    void add(const Suffix& suffix) noexcept {
        for (int i = 0; i < Sigma; ++i) {
            const uint8_t byte = suffix[i];
            low_nibbles_[i] |= nibble_bit(byte & 0x0F);
            high_nibbles_[i] |= nibble_bit(byte >> 4);
        }
        update_value();
    }

    void merge(const NibbleCountScore& other) noexcept {
        for (int i = 0; i < Sigma; ++i) {
            low_nibbles_[i] |= other.low_nibbles_[i];
            high_nibbles_[i] |= other.high_nibbles_[i];
        }
        update_value();
    }

    [[nodiscard]] uint64_t value() const noexcept { return value_; }

   private:
    static uint16_t nibble_bit(uint8_t nibble) noexcept {
        return static_cast<uint16_t>(uint16_t{1} << nibble);
    }

    void update_value() noexcept {
        value_ = 1;
        for (int i = 0; i < Sigma; ++i) {
            value_ *= std::popcount(static_cast<unsigned int>(low_nibbles_[i]));
            value_ *=
                std::popcount(static_cast<unsigned int>(high_nibbles_[i]));
        }
    }

    std::array<uint16_t, Sigma> low_nibbles_{};
    std::array<uint16_t, Sigma> high_nibbles_{};
    uint64_t value_ = 0;
};

}  // namespace teddy::grouping
