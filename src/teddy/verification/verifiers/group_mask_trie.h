#pragma once

#include "findkey.h"
#include "teddy/verification/json_context.h"
#include "teddy/verification/verifier.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace teddy {

class GroupMaskTrieVerifier final {
   public:
    static constexpr findkey_teddy_verification_strategy strategy =
        TEDDY_VERIFY_GROUP_MASK_TRIE;

    explicit GroupMaskTrieVerifier(const VerificationBuildContext& context);

    CandidateResult check(std::string_view input,
                          size_t end_quote,
                          uint8_t candidate_groups) const {
        const char* str = input.data();
        int32_t current_node = 0;
        size_t consumed = 0;

        if ((nodes_[current_node].group_mask & candidate_groups) == 0) {
            return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
        }

        for (size_t position = end_quote; position > 0;) {
            --position;
            const uint8_t c = static_cast<uint8_t>(str[position]);

            if (c == '"' && is_valid_quote(str, position)) {
                const TrieNode& node = nodes_[current_node];
                if (node.key_id != -1 &&
                    (node.terminal_group_mask & candidate_groups) != 0) {
                    return {
                        CANDIDATE_MATCH,
                        position + 1,
                        static_cast<uint32_t>(node.key_id),
                    };
                }
                return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
            }

            if (consumed >= max_key_len_) {
                return {CANDIDATE_MISSING_OPEN_QUOTE, 0, 0};
            }

            const int32_t next_node = nodes_[current_node].children[c];
            if (next_node == -1) {
                return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
            }

            current_node = next_node;
            if ((nodes_[current_node].group_mask & candidate_groups) == 0) {
                return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
            }
            ++consumed;
        }

        return {CANDIDATE_MISSING_OPEN_QUOTE, 0, 0};
    }

    size_t size() const noexcept { return nodes_.size(); }
    size_t max_key_len() const noexcept { return max_key_len_; }

   private:
    struct TrieNode {
        std::array<int32_t, 256> children{};
        int32_t key_id = -1;
        uint8_t group_mask = 0;
        uint8_t terminal_group_mask = 0;

        TrieNode() { children.fill(-1); }
    };

    std::vector<TrieNode> nodes_;
    size_t max_key_len_ = 0;
};

static_assert(Verifier<GroupMaskTrieVerifier>);

}  // namespace teddy
