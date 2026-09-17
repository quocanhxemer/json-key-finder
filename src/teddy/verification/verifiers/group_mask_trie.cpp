#include "teddy/verification/verifiers/group_mask_trie.h"

#include "teddy/compile.h"
#include "teddy/verification/group_mapping.h"

#include <cstdint>
#include <vector>

namespace teddy {

GroupMaskTrieVerifier::GroupMaskTrieVerifier(
    const VerificationBuildContext& context) {
    const CompilationData& teddy = context.teddy;

    const std::vector<uint8_t> suffix_group_masks =
        verification::detail::build_suffix_group_masks(teddy);

    for (uint32_t key_id = 0; key_id < context.keys.size(); ++key_id) {
        const uint32_t suffix_id = teddy.key_suffix_ids[key_id];
        insert(context.keys[key_id], key_id, suffix_group_masks[suffix_id]);
    }
}

}  // namespace teddy
