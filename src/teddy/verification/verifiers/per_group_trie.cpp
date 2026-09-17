#include "teddy/verification/verifiers/per_group_trie.h"

#include "teddy/compile.h"
#include "teddy/verification/group_mapping.h"

#include <cstdint>
#include <vector>

namespace teddy {

PerGroupTrieVerifier::PerGroupTrieVerifier(
    const VerificationBuildContext& context) {
    const CompilationData& teddy = context.teddy;
    group_tries_.resize(teddy.num_groups());

    const std::vector<uint8_t> suffix_group_ids =
        verification::detail::build_suffix_group_ids(teddy);

    for (uint32_t key_id = 0; key_id < context.keys.size(); ++key_id) {
        const uint32_t suffix_id = teddy.key_suffix_ids[key_id];
        group_tries_[suffix_group_ids[suffix_id]].insert(
            context.keys[key_id], key_id,
            verification::detail::PlainTriePolicy::InsertMetadata{});
    }
}

}  // namespace teddy
