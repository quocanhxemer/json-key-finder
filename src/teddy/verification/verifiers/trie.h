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

struct TrieNode {
    std::array<int32_t, 256> children{};
    int32_t key_id = -1;

    TrieNode() { children.fill(-1); }
};

class TrieVerifier final {
   public:
    static constexpr findkey_teddy_verification_strategy strategy =
        TEDDY_VERIFY_TRIE;

    explicit TrieVerifier(const std::vector<std::string_view>& keys);

    candidate_result check(std::string_view input, size_t end_quote) const {
        const char* str = input.data();
        int32_t current_node = 0;
        size_t consumed = 0;

        for (size_t position = end_quote; position > 0;) {
            --position;
            const uint8_t c = static_cast<uint8_t>(str[position]);

            if (c == '"' && is_valid_quote(str, position)) {
                if (nodes_[current_node].key_id != -1) {
                    return {
                        CANDIDATE_TYPE_MATCH,
                        position + 1,
                        static_cast<uint32_t>(nodes_[current_node].key_id),
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
            ++consumed;
        }

        return {CANDIDATE_MISSING_OPEN_QUOTE, 0, 0};
    }

    size_t size() const noexcept { return nodes_.size(); }
    size_t max_key_len() const noexcept { return max_key_len_; }

   private:
    std::vector<TrieNode> nodes_;
    size_t max_key_len_ = 0;
};

static_assert(Verifier<TrieVerifier>);

}  // namespace teddy
