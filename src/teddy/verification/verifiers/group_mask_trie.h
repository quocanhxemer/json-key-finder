#pragma once

#include "findkey.h"
#include "teddy/verification/verifier.h"
#include "teddy/verification/verifiers/reverse_trie.h"

#include <array>
#include <cstdint>

namespace teddy::verification::detail {

struct GroupMaskTriePolicy {
    using InsertMetadata = uint8_t;

    struct Node {
        std::array<int32_t, 256> children{};
        int32_t key_id = -1;

        // valid group bits at current node
        uint8_t group_mask = 0;

        // valid group bits at terminal node
        uint8_t terminal_group_mask = 0;

        Node() { children.fill(-1); }
    };

    static void mark_node(Node& node, uint8_t group_bit) noexcept {
        node.group_mask |= group_bit;
    }

    static void mark_terminal(Node& node, uint8_t group_bit) noexcept {
        node.terminal_group_mask |= group_bit;
    }

    static bool can_match_subtree(const Node& node,
                                  uint8_t candidate_groups) noexcept {
        return (node.group_mask & candidate_groups) != 0;
    }

    static bool can_match_terminal(const Node& node,
                                   uint8_t candidate_groups) noexcept {
        return (node.terminal_group_mask & candidate_groups) != 0;
    }
};

static_assert(ReverseTrieNode<GroupMaskTriePolicy::Node>);

using GroupMaskReverseTrie = ReverseTrie<GroupMaskTriePolicy>;

}  // namespace teddy::verification::detail

namespace teddy {

class GroupMaskTrieVerifier final
    : private verification::detail::GroupMaskReverseTrie {
    using Base = verification::detail::GroupMaskReverseTrie;

   public:
    static constexpr findkey_teddy_verification_strategy strategy =
        TEDDY_VERIFY_GROUP_MASK_TRIE;

    explicit GroupMaskTrieVerifier(const VerificationBuildContext& context);

    using Base::check;
    using Base::memory_usage_bytes;
};

static_assert(sizeof(GroupMaskTrieVerifier) ==
              sizeof(verification::detail::GroupMaskReverseTrie));
static_assert(Verifier<GroupMaskTrieVerifier>);

}  // namespace teddy
