#pragma once

#include "findkey.h"
#include "teddy/verification/verifiers/hash.h"

#include <string_view>
#include <vector>

/*
    - Scan the data to find JSON keys
        i.e. enclosed in double quotes and followed by a colon (:)
    - Delegate exact key lookup to the hash verifier
*/
std::vector<findkey_result> matcher_scalar(std::string_view data,
                                           const teddy::HashVerifier& verifier);
