#include "teddy/verification/verifiers/group_mask_trie.h"

#include "teddy/compile.h"

#include <cstdint>
#include <vector>

namespace teddy {

GroupMaskTrieVerifier::GroupMaskTrieVerifier(
    const VerificationBuildContext& context) {
    const CompilationData& teddy = context.teddy;

    // maps suffix ID to group bitmask
    std::vector<uint8_t> suffix_group_masks(teddy.suffixes.size(), 0);
    for (size_t group = 0; group < teddy.group_suffix_ids.size(); ++group) {
        const uint8_t group_bit = static_cast<uint8_t>(uint8_t{1} << group);
        for (const uint32_t suffix_id : teddy.group_suffix_ids[group]) {
            suffix_group_masks[suffix_id] = group_bit;
        }
    }

    for (uint32_t key_id = 0; key_id < context.keys.size(); ++key_id) {
        const uint32_t suffix_id = teddy.key_suffix_ids[key_id];
        insert(context.keys[key_id], key_id, suffix_group_masks[suffix_id]);
    }
}

}  // namespace teddy
