#pragma once

#include "core/findkey_error.h"
#include "findkey.h"
#include "teddy/verification/verifiers/group_mask_trie.h"
#include "teddy/verification/verifiers/hash.h"
#include "teddy/verification/verifiers/per_group_trie.h"
#include "teddy/verification/verifiers/per_suffix_trie.h"
#include "teddy/verification/verifiers/plain_trie.h"

#include <utility>

namespace teddy {

template <typename Function>
decltype(auto) dispatch_verifier(findkey_teddy_verification_strategy strategy,
                                 Function&& function) {
    switch (strategy) {
        case TEDDY_VERIFY_HASH:
            return std::forward<Function>(function)
                .template operator()<HashVerifier>();
        case TEDDY_VERIFY_PLAIN_TRIE:
            return std::forward<Function>(function)
                .template operator()<PlainTrieVerifier>();
        case TEDDY_VERIFY_GROUP_MASK_TRIE:
            return std::forward<Function>(function)
                .template operator()<GroupMaskTrieVerifier>();
        case TEDDY_VERIFY_PER_GROUP_TRIE:
            return std::forward<Function>(function)
                .template operator()<PerGroupTrieVerifier>();
        case TEDDY_VERIFY_PER_SUFFIX_TRIE:
            return std::forward<Function>(function)
                .template operator()<PerSuffixTrieVerifier>();
        default:
            throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                               "Unknown Teddy verification strategy");
    }
}

}  // namespace teddy
