#pragma once

#include "findkey.h"

#include <string_view>
#include <vector>

std::vector<findkey_result> matcher_scalar(
    std::string_view data,
    const std::vector<std::string_view>& keys);
