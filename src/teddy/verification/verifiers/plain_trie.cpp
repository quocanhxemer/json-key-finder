#include "teddy/verification/verifiers/plain_trie.h"

#include <cstdint>

namespace teddy {

PlainTrieVerifier::PlainTrieVerifier(const VerificationBuildContext& context) {
    for (uint32_t key_id = 0; key_id < context.keys.size(); ++key_id) {
        insert(context.keys[key_id], key_id,
               verification::detail::PlainTriePolicy::InsertMetadata{});
    }
}

}  // namespace teddy
