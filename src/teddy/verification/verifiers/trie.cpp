#include "teddy/verification/verifiers/trie.h"

#include <algorithm>

namespace teddy {

TrieVerifier::TrieVerifier(const std::vector<std::string_view>& keys) {
    nodes_.emplace_back();  // root

    for (uint32_t key_id = 0; key_id < keys.size(); ++key_id) {
        const std::string_view key = keys[key_id];
        max_key_len_ = std::max(max_key_len_, key.size());

        int32_t current_node = 0;
        for (size_t i = key.size(); i > 0; --i) {
            const uint8_t c = static_cast<uint8_t>(key[i - 1]);
            if (nodes_[current_node].children[c] == -1) {
                nodes_[current_node].children[c] =
                    static_cast<int32_t>(nodes_.size());
                nodes_.emplace_back();
            }
            current_node = nodes_[current_node].children[c];
        }

        if (nodes_[current_node].key_id == -1) {
            nodes_[current_node].key_id = static_cast<int32_t>(key_id);
        }
    }
}

}  // namespace teddy
