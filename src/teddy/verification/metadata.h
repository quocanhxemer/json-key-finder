#pragma once

#include "findkey.h"
#include "teddy/verification/verifier.h"

#include <cstddef>

namespace teddy {

struct VerifierCompilationMetadata {
    findkey_teddy_verification_strategy strategy = TEDDY_VERIFY_PLAIN_TRIE;
    // Estimated object and owned-container storage
    size_t verifier_size_bytes = 0;
};

template <Verifier VerifierModel>
VerifierCompilationMetadata get_verifier_compilation_metadata(
    const VerifierModel& verifier) {
    return {
        .strategy = VerifierModel::strategy,
        .verifier_size_bytes = verifier.memory_usage_bytes(),
    };
}

}  // namespace teddy
