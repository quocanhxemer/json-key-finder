#pragma once

#include "teddy/compile.h"
#include "teddy/verification/json_context.h"
#include "teddy/verification/result.h"
#include "teddy/verification/verifier.h"

#include <cctype>
#include <cstdint>
#include <vector>

namespace teddy {

template <int Sigma>
static inline bool group_has_exact_suffix(const CompilationData& data,
                                          uint32_t group,
                                          const uint8_t* suffix) {
    for (uint32_t suffix_id : data.group_suffix_ids[group]) {
        bool found = true;
        for (int i = 0; i < Sigma; ++i) {
            if (data.suffixes[suffix_id][i] != suffix[i]) {
                found = false;
                break;
            }
        }

        if (found) {
            return true;
        }
    }

    return false;
}

template <Verifier VerifierModel>
static inline CandidateResult verify_json_key_candidate(
    std::string_view input,
    size_t end_quote,
    uint8_t candidate_groups,
    const VerifierModel& verifier) {
    const char* str = input.data();
    const size_t len = input.size();

    if (end_quote >= len || str[end_quote] != '"') {
        return {CANDIDATE_BAD_END_QUOTE, 0, 0};
    }

    if (!is_valid_quote(str, end_quote)) {
        return {CANDIDATE_INVALID_QUOTE, 0, 0};
    }

    size_t j = end_quote + 1;
    while (j < len && std::isspace(static_cast<unsigned char>(str[j]))) {
        ++j;
    }

    if (j >= len || str[j] != ':') {
        return {CANDIDATE_MISSING_COLON, 0, 0};
    }

    return verifier.check(input, end_quote, candidate_groups);
}

}  // namespace teddy
