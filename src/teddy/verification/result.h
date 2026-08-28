#pragma once

#include <cstddef>
#include <cstdint>

namespace teddy {

enum candidate_type {
    CANDIDATE_TYPE_MATCH = 0,
    CANDIDATE_BAD_END_QUOTE = 1,
    CANDIDATE_INVALID_QUOTE = 2,
    CANDIDATE_MISSING_COLON = 3,
    CANDIDATE_MISSING_OPEN_QUOTE = 4,
    CANDIDATE_KEY_NOT_FOUND = 5,
};

struct candidate_result {
    candidate_type type = CANDIDATE_TYPE_MATCH;
    size_t position = 0;
    uint32_t key_id = 0;
};

}  // namespace teddy
