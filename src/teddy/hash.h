#pragma once

#include "core/findkey_error.h"
#include "findkey.h"

#include <xxhash.h>
#include <zlib.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

namespace teddy {

template <size_t Length>
uint32_t hash_grouping_bytes(
    const uint8_t* data,
    enum findkey_teddy_compile_grouping_strategy grouping_strategy) {
    static_assert(Length > 0 && Length <= FINDKEY_TEDDY_MAX_SIGMA,
                  "Teddy hash length is out of range");

    switch (grouping_strategy) {
        case TEDDY_COMPILE_HASH_STD:
            return static_cast<uint32_t>(std::hash<std::string_view>{}(
                std::string_view(reinterpret_cast<const char*>(data), Length)));
        case TEDDY_COMPILE_HASH_ADLER32:
            return static_cast<uint32_t>(
                ::adler32(1L, data, static_cast<uInt>(Length)));
        case TEDDY_COMPILE_HASH_CRC32:
            return static_cast<uint32_t>(
                ::crc32(0L, data, static_cast<uInt>(Length)));
        case TEDDY_COMPILE_HASH_XXHASH:
            return static_cast<uint32_t>(XXH32(data, Length, 0u));
        case TEDDY_COMPILE_HASH_FNV1A: {
            uint32_t hash = 2166136261u;
            for (size_t i = 0; i < Length; ++i) {
                hash ^= data[i];
                hash *= 16777619u;
            }
            return hash;
        }
        default:
            throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                               "Invalid Teddy hash grouping strategy");
    }
}

}  // namespace teddy
