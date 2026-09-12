#pragma once

#include "findkey.h"
#include "teddy/verification/json_context.h"
#include "teddy/verification/verifier.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace teddy {

class HashVerifier final {
   public:
    static constexpr findkey_teddy_verification_strategy strategy =
        TEDDY_VERIFY_HASH;

    explicit HashVerifier(const VerificationBuildContext& context);

    CandidateResult check(std::string_view input,
                          size_t end_quote,
                          uint8_t candidate_groups) const {
        (void)candidate_groups;
        const char* str = input.data();

        size_t min_start_quote = 0;
        if (end_quote > max_key_len_ + 1) {
            min_start_quote = end_quote - max_key_len_ - 1;
        }

        for (size_t position = end_quote; position > min_start_quote;) {
            --position;
            if (str[position] == '"' && is_valid_quote(str, position)) {
                const std::string_view key(str + position + 1,
                                           end_quote - position - 1);
                const auto key_it = keys_.find(key);
                if (key_it != keys_.end()) {
                    return {CANDIDATE_MATCH, position + 1, key_it->second};
                }
                return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
            }
        }

        return {CANDIDATE_MISSING_OPEN_QUOTE, 0, 0};
    }

    size_t size() const noexcept { return keys_.size(); }

   private:
    std::unordered_map<std::string_view, uint32_t> keys_;
    size_t max_key_len_ = 0;
};

static_assert(Verifier<HashVerifier>);

}  // namespace teddy
