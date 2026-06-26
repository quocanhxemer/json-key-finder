#pragma once

#include "findkey.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

static inline size_t teddy_virtual_length(
    std::string_view key,
    enum findkey_teddy_suffix_mode suffix_mode) {
    switch (suffix_mode) {
        case TEDDY_SUFFIX_RAW:
            return key.size();
        case TEDDY_SUFFIX_QUOTED:
            return key.size() + 1;
        default:
            return -1;
    }
}

static inline uint8_t teddy_suffix_byte(
    std::string_view key,
    int sigma,
    int i,
    enum findkey_teddy_suffix_mode suffix_mode) {
    const size_t virtual_len = teddy_virtual_length(key, suffix_mode);
    const size_t idx = virtual_len - sigma + i;

    if (idx < key.size()) {
        return static_cast<uint8_t>(key[idx]);
    }

    // for QUOTED mode, the virtual suffix byte after the last character is
    // the closing quote (")
    return '"';
}
