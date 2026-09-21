#pragma once

#include "findkey.h"
#include "teddy/verification/memory_usage.h"
#include "teddy/verification/verifier.h"
#include "teddy/verification/verifiers/plain_trie.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace teddy {

class PerSuffixTrieVerifier final {
    using Trie = verification::detail::PlainReverseTrie;

   public:
    static constexpr findkey_teddy_verification_strategy strategy =
        TEDDY_VERIFY_PER_SUFFIX_TRIE;

    explicit PerSuffixTrieVerifier(const VerificationBuildContext& context);

    CandidateResult check(std::string_view input,
                          size_t end_quote,
                          uint8_t candidate_groups) const;

    size_t memory_usage_bytes() const noexcept;

   private:
    std::vector<Trie> suffix_tries_;
    std::unordered_map<uint64_t, size_t> suffix_trie_ids_;
    int sigma_ = 0;
    size_t end_quote_offset_ = 0;
};

static_assert(Verifier<PerSuffixTrieVerifier>);

}  // namespace teddy
