#include "teddy/verification/verifiers/group_mask_trie.h"

#include "core/findkey_error.h"
#include "teddy/compile.h"

#include <algorithm>
#include <limits>

namespace teddy {

GroupMaskTrieVerifier::GroupMaskTrieVerifier(
    const VerificationBuildContext& context) {
    const std::vector<std::string_view>& keys = context.keys;
    const CompilationData& teddy = context.teddy;

    std::vector<uint8_t> suffix_group_masks(teddy.suffixes.size(), 0);
    for (size_t group = 0; group < teddy.group_suffix_ids.size(); ++group) {
        const uint8_t group_bit = static_cast<uint8_t>(uint8_t{1} << group);
        for (const uint32_t suffix_id : teddy.group_suffix_ids[group]) {
            suffix_group_masks[suffix_id] = group_bit;
        }
    }

    nodes_.emplace_back();  // root

    for (uint32_t key_id = 0; key_id < keys.size(); ++key_id) {
        const uint32_t suffix_id = teddy.key_suffix_ids[key_id];
        const uint8_t group_bit = suffix_group_masks[suffix_id];
        const std::string_view key = keys[key_id];
        max_key_len_ = std::max(max_key_len_, key.size());

        int32_t current_node = 0;
        nodes_[current_node].group_mask |= group_bit;

        for (size_t i = key.size(); i > 0; --i) {
            const uint8_t c = static_cast<uint8_t>(key[i - 1]);
            if (nodes_[current_node].children[c] == -1) {
                nodes_[current_node].children[c] =
                    static_cast<int32_t>(nodes_.size());
                nodes_.emplace_back();
            }
            current_node = nodes_[current_node].children[c];
            nodes_[current_node].group_mask |= group_bit;
        }

        TrieNode& terminal = nodes_[current_node];
        terminal.terminal_group_mask |= group_bit;
        if (terminal.key_id == -1) {
            terminal.key_id = static_cast<int32_t>(key_id);
        }
    }
}

}  // namespace teddy
