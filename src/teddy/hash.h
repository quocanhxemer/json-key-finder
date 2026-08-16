#pragma once

#include "findkey.h"

#include <cstddef>
#include <cstdint>

namespace teddy {

uint32_t hash_grouping_bytes(
    const uint8_t* data,
    size_t len,
    enum findkey_teddy_compile_grouping_strategy grouping_strategy);

}  // namespace teddy
