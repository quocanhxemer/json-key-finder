#pragma once

#include "findkey.h"
#include "teddy/teddy_common.h"

#include <string_view>
#include <vector>

/*
    - Scan blocks of data to find potential matches using Teddy algorithm
    - For each potential match, veryfy if it's a valid key in JSON format
        i.e. enclosed in double quotes and followed by a colon (:)
    - Then check if the key exists in the keys list using a hash map
*/

// Compile keys and call matcher_teddy_baseline_compiled
std::vector<findkey_result> matcher_teddy(
    std::string_view data,
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config);

std::vector<findkey_result> matcher_teddy_compiled(
    std::string_view data,
    const TeddyCompilationData& teddy_data);
