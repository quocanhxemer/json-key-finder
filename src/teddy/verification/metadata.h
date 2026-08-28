#pragma once

#include "findkey.h"
#include "teddy/verification/verifier.h"

#include <cstddef>

namespace teddy {

struct VerifierCompilationMetadata {
    findkey_teddy_verification_strategy strategy = TEDDY_VERIFY_TRIE;
    size_t trie_nodes = 0;
    size_t hash_keys = 0;
    size_t max_key_len = 0;
};

template <Verifier VerifierModel>
VerifierCompilationMetadata get_verifier_compilation_metadata(
    const VerifierModel& verifier) {
    VerifierCompilationMetadata metadata{
        .strategy = VerifierModel::strategy,
        .max_key_len = verifier.max_key_len(),
    };
    if constexpr (VerifierModel::strategy == TEDDY_VERIFY_TRIE) {
        metadata.trie_nodes = verifier.size();
    } else {
        metadata.hash_keys = verifier.size();
    }
    return metadata;
}

}  // namespace teddy
