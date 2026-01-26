#pragma once

#include "findkey.h"

#include <string_view>
#include <vector>

/*
    - Scan the data to find JSON keys
        i.e. enclosed in double quotes and followed by a colon (:)
    - Then check if the key exists in the keys list using a hash map
*/
std::vector<findkey_result> matcher_scalar(
    std::string_view data,
    const std::vector<std::string_view>& keys);
