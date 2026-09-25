#pragma once

#include <cstdint>

namespace json_syntax {

// RFC whitespace
[[nodiscard]] constexpr bool is_whitespace(uint8_t byte) noexcept {
    return byte == 0x20 || byte == 0x09 || byte == 0x0A || byte == 0x0D;
}

}  // namespace json_syntax
