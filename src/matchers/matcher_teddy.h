#pragma once

#include "findkey.h"

#include <string_view>
#include <vector>

/*
    - Scan blocks of data to find potential matches using Teddy algorithm
    - For each potential match, veryfy if it's a valid key in JSON format
        i.e. enclosed in double quotes and followed by a colon (:)
    - Then check if the key exists in the keys list using a hash map
*/
std::vector<findkey_result> matcher_teddy(
    std::string_view data,
    const std::vector<std::string_view>& keys);
