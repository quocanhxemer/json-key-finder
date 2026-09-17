#pragma once

#include "findkey.h"
#include "teddy/verification/verifier.h"
#include "teddy/verification/verifiers/plain_trie.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace teddy {

class PerGroupTrieVerifier final {
    using Trie = verification::detail::PlainReverseTrie;

   public:
    static constexpr findkey_teddy_verification_strategy strategy =
        TEDDY_VERIFY_PER_GROUP_TRIE;

    explicit PerGroupTrieVerifier(const VerificationBuildContext& context);

    CandidateResult check(std::string_view input,
                          size_t end_quote,
                          uint8_t candidate_groups) const {
        CandidateResult rejection{CANDIDATE_KEY_NOT_FOUND, 0, 0};

        while (candidate_groups) {
            const unsigned group =
                std::countr_zero(static_cast<unsigned>(candidate_groups));

            if (group >= group_tries_.size()) {
                break;
            }

            candidate_groups &= candidate_groups - 1;

            const CandidateResult result = group_tries_[group].check(
                input, end_quote, static_cast<uint8_t>(uint8_t{1} << group));
            if (result.type == CANDIDATE_MATCH) {
                return result;
            }
            if (result.type == CANDIDATE_MISSING_OPEN_QUOTE) {
                rejection = result;
            }
        }

        return rejection;
    }

    size_t size() const noexcept {
        size_t total_nodes = 0;
        for (const Trie& trie : group_tries_) {
            total_nodes += trie.size();
        }
        return total_nodes;
    }

   private:
    std::vector<Trie> group_tries_;
};

static_assert(Verifier<PerGroupTrieVerifier>);
static_assert(sizeof(PerGroupTrieVerifier) ==
              sizeof(std::vector<verification::detail::PlainReverseTrie>));

}  // namespace teddy
