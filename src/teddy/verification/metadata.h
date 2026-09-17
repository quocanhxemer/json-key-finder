#pragma once

#include "findkey.h"
#include "teddy/verification/verifier.h"

#include <cstddef>

namespace teddy {

struct VerifierCompilationMetadata {
    findkey_teddy_verification_strategy strategy = TEDDY_VERIFY_PLAIN_TRIE;
    size_t trie_nodes = 0;
    size_t hash_keys = 0;
};

template <Verifier VerifierModel>
VerifierCompilationMetadata get_verifier_compilation_metadata(
    const VerifierModel& verifier) {
    VerifierCompilationMetadata metadata{
        .strategy = VerifierModel::strategy,
    };

    constexpr auto strategy = VerifierModel::strategy;
    switch (strategy) {
        case TEDDY_VERIFY_HASH:
            metadata.hash_keys = verifier.size();
            break;
        case TEDDY_VERIFY_PLAIN_TRIE:
        case TEDDY_VERIFY_GROUP_MASK_TRIE:
        case TEDDY_VERIFY_PER_GROUP_TRIE:
            metadata.trie_nodes = verifier.size();
            break;
        case FINDKEY_TEDDY_VERIFICATION_STRATEGY_COUNT:
            // shouldn't happen
            break;
    }
    return metadata;
}

}  // namespace teddy
