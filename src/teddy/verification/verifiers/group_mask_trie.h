#pragma once

#include "findkey.h"
#include "teddy/verification/verifier.h"
#include "teddy/verification/verifiers/reverse_trie.h"

namespace teddy {

class GroupMaskTrieVerifier final
    : private detail::ReverseTrie<detail::GroupMaskTriePolicy> {
    using Base = detail::ReverseTrie<detail::GroupMaskTriePolicy>;

   public:
    static constexpr findkey_teddy_verification_strategy strategy =
        TEDDY_VERIFY_GROUP_MASK_TRIE;

    explicit GroupMaskTrieVerifier(const VerificationBuildContext& context);

    using Base::check;
    using Base::max_key_len;
    using Base::size;
};

static_assert(sizeof(GroupMaskTrieVerifier) ==
              sizeof(detail::ReverseTrie<detail::GroupMaskTriePolicy>));
static_assert(Verifier<GroupMaskTrieVerifier>);

}  // namespace teddy
