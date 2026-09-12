#pragma once

#include "findkey.h"
#include "teddy/verification/verifier.h"
#include "teddy/verification/verifiers/reverse_trie.h"

#include <array>
#include <cstdint>

namespace teddy::verification::detail {

struct PlainTriePolicy {
    struct InsertMetadata {};

    struct Node {
        std::array<int32_t, 256> children{};
        int32_t key_id = -1;

        Node() { children.fill(-1); }
    };

    static constexpr void mark_node(Node&, InsertMetadata) noexcept {}
    static constexpr void mark_terminal(Node&, InsertMetadata) noexcept {}
    static constexpr bool can_match_subtree(const Node&, uint8_t) noexcept {
        return true;
    }
    static constexpr bool can_match_terminal(const Node&, uint8_t) noexcept {
        return true;
    }
};

static_assert(ReverseTrieNode<PlainTriePolicy::Node>);
static_assert(sizeof(PlainTriePolicy::Node) ==
              sizeof(std::array<int32_t, 256>) + sizeof(int32_t));

}  // namespace teddy::verification::detail

namespace teddy {

class PlainTrieVerifier final : private verification::detail::ReverseTrie<
                                    verification::detail::PlainTriePolicy> {
    using Base = verification::detail::ReverseTrie<
        verification::detail::PlainTriePolicy>;

   public:
    static constexpr findkey_teddy_verification_strategy strategy =
        TEDDY_VERIFY_TRIE;

    explicit PlainTrieVerifier(const VerificationBuildContext& context);

    using Base::check;
    using Base::size;
};

static_assert(sizeof(PlainTrieVerifier) ==
              sizeof(verification::detail::ReverseTrie<
                     verification::detail::PlainTriePolicy>));
static_assert(Verifier<PlainTrieVerifier>);

}  // namespace teddy
