#include "teddy/teddy_hash.h"

#include <xxhash.h>
#include <zlib.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

static inline uint32_t fnv1a_hash(const uint8_t* data, size_t len) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

uint32_t hash_teddy_suffix(
    const uint8_t* suffix,
    size_t len,
    enum findkey_teddy_compile_grouping_strategy grouping_strategy) {
    switch (grouping_strategy) {
        case TEDDY_COMPILE_HASH_STD:
            return static_cast<uint32_t>(std::hash<std::string_view>{}(
                std::string_view(reinterpret_cast<const char*>(suffix), len)));
        case TEDDY_COMPILE_HASH_ADLER32:
            return static_cast<uint32_t>(
                ::adler32(1L, suffix, static_cast<uInt>(len)));
        case TEDDY_COMPILE_HASH_CRC32:
            return static_cast<uint32_t>(
                ::crc32(0L, suffix, static_cast<uInt>(len)));
        case TEDDY_COMPILE_HASH_XXHASH:
            return static_cast<uint32_t>(XXH32(suffix, len, 0u));
        case TEDDY_COMPILE_HASH_FNV1A:
            return fnv1a_hash(suffix, len);
        default:
            return 0;
    }
}
