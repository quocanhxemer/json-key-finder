#pragma once

#include "findkey.h"

#include <cstddef>
#include <cstdint>

uint32_t hash_teddy_suffix(
    const uint8_t* suffix,
    size_t len,
    enum findkey_teddy_compile_grouping_strategy grouping_strategy);
