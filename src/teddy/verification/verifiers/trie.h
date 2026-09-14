#pragma once

#include "findkey.h"
#include "teddy/verification/verifiers/reverse_trie.h"
#include "teddy/verification/verifier.h"

namespace teddy {

class TrieVerifier final
    : private detail::ReverseTrie<detail::PlainTriePolicy> {
    using Base = detail::ReverseTrie<detail::PlainTriePolicy>;

   public:
    static constexpr findkey_teddy_verification_strategy strategy =
        TEDDY_VERIFY_TRIE;

    explicit TrieVerifier(const VerificationBuildContext& context);

    using Base::check;
    using Base::max_key_len;
    using Base::size;
};

static_assert(sizeof(TrieVerifier) ==
              sizeof(detail::ReverseTrie<detail::PlainTriePolicy>));
static_assert(Verifier<TrieVerifier>);

}  // namespace teddy
