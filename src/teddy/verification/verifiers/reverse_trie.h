#pragma once

#include "teddy/verification/json_context.h"
#include "teddy/verification/memory_usage.h"
#include "teddy/verification/result.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace teddy::verification::detail {

template <typename Node>
concept ReverseTrieNode =
    std::default_initializable<Node> && std::movable<Node> &&
    requires(Node& node, const Node& const_node, uint8_t byte) {
        { node.children[byte] } -> std::same_as<int32_t&>;
        { const_node.children[byte] } -> std::same_as<const int32_t&>;
        { node.key_id } -> std::same_as<int32_t&>;
        { const_node.key_id } -> std::same_as<const int32_t&>;
    };

template <typename Policy>
    requires ReverseTrieNode<typename Policy::Node>
class ReverseTrie {
   public:
    ReverseTrie() { nodes_.emplace_back(); }

    void insert(std::string_view key,
                uint32_t key_id,
                typename Policy::InsertMetadata metadata) {
        int32_t current_node = 0;
        Policy::mark_node(nodes_[current_node], metadata);

        for (size_t i = key.size(); i > 0; --i) {
            const uint8_t c = static_cast<uint8_t>(key[i - 1]);
            if (nodes_[current_node].children[c] == -1) {
                nodes_[current_node].children[c] =
                    static_cast<int32_t>(nodes_.size());
                nodes_.emplace_back();
            }
            current_node = nodes_[current_node].children[c];
            Policy::mark_node(nodes_[current_node], metadata);
        }

        Policy::mark_terminal(nodes_[current_node], metadata);
        if (nodes_[current_node].key_id == -1) {
            nodes_[current_node].key_id = static_cast<int32_t>(key_id);
        }
    }

    CandidateResult check(std::string_view input,
                          size_t start_position,
                          uint8_t candidate_groups) const {
        const char* str = input.data();
        int32_t current_node = 0;

        if (!Policy::can_match_subtree(nodes_[current_node],
                                       candidate_groups)) {
            return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
        }

        for (size_t position = start_position; position > 0;) {
            --position;
            const uint8_t c = static_cast<uint8_t>(str[position]);

            if (c == '"' && is_valid_quote(str, position)) {
                const Node& node = nodes_[current_node];
                if (node.key_id != -1 &&
                    Policy::can_match_terminal(node, candidate_groups)) {
                    return {
                        CANDIDATE_MATCH,
                        position + 1,
                        static_cast<uint32_t>(node.key_id),
                    };
                }
                return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
            }

            const int32_t next_node = nodes_[current_node].children[c];
            if (next_node == -1) {
                return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
            }

            current_node = next_node;
            if (!Policy::can_match_subtree(nodes_[current_node],
                                           candidate_groups)) {
                return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
            }
        }

        return {CANDIDATE_MISSING_OPEN_QUOTE, 0, 0};
    }

    size_t memory_usage_bytes() const noexcept {
        return sizeof(*this) + dynamic_memory_usage_bytes();
    }

    size_t dynamic_memory_usage_bytes() const noexcept {
        return vector_allocation_bytes(nodes_);
    }

   private:
    using Node = typename Policy::Node;

    std::vector<Node> nodes_;
};

}  // namespace teddy::verification::detail
