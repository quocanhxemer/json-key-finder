#pragma once

#include "teddy/verification/json_context.h"
#include "teddy/verification/verifier.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace teddy::detail {

struct NoTrieMetadata {};

struct PlainTriePolicy {
    using InsertMetadata = NoTrieMetadata;
    static constexpr bool filters_candidates = false;

    struct Node {
        std::array<int32_t, 256> children{};
        int32_t key_id = -1;

        Node() { children.fill(-1); }
    };

    static constexpr void mark_node(Node&, InsertMetadata) noexcept {}
    static constexpr void mark_terminal(Node&, InsertMetadata) noexcept {}
};

struct GroupMaskTriePolicy {
    using InsertMetadata = uint8_t;
    static constexpr bool filters_candidates = true;

    struct Node {
        std::array<int32_t, 256> children{};
        int32_t key_id = -1;
        uint8_t group_mask = 0;
        uint8_t terminal_group_mask = 0;

        Node() { children.fill(-1); }
    };

    static void mark_node(Node& node, uint8_t group_bit) noexcept {
        node.group_mask |= group_bit;
    }

    static void mark_terminal(Node& node, uint8_t group_bit) noexcept {
        node.terminal_group_mask |= group_bit;
    }

    static bool allows_node(const Node& node,
                            uint8_t candidate_groups) noexcept {
        return (node.group_mask & candidate_groups) != 0;
    }

    static bool allows_terminal(const Node& node,
                                uint8_t candidate_groups) noexcept {
        return (node.terminal_group_mask & candidate_groups) != 0;
    }
};

static_assert(sizeof(PlainTriePolicy::Node) ==
              sizeof(std::array<int32_t, 256>) + sizeof(int32_t));

template <typename Policy>
class ReverseTrie {
   public:
    ReverseTrie() { nodes_.emplace_back(); }

    void insert(std::string_view key,
                uint32_t key_id,
                typename Policy::InsertMetadata metadata) {
        max_key_len_ = std::max(max_key_len_, key.size());

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
                          size_t end_quote,
                          uint8_t candidate_groups) const {
        const char* str = input.data();
        int32_t current_node = 0;
        size_t consumed = 0;

        if constexpr (Policy::filters_candidates) {
            if (!Policy::allows_node(nodes_[current_node], candidate_groups)) {
                return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
            }
        } else {
            (void)candidate_groups;
        }

        for (size_t position = end_quote; position > 0;) {
            --position;
            const uint8_t c = static_cast<uint8_t>(str[position]);

            if (c == '"' && is_valid_quote(str, position)) {
                if constexpr (Policy::filters_candidates) {
                    const Node& node = nodes_[current_node];
                    if (node.key_id != -1 &&
                        Policy::allows_terminal(node, candidate_groups)) {
                        return {
                            CANDIDATE_MATCH,
                            position + 1,
                            static_cast<uint32_t>(node.key_id),
                        };
                    }
                } else {
                    if (nodes_[current_node].key_id != -1) {
                        return {
                            CANDIDATE_MATCH,
                            position + 1,
                            static_cast<uint32_t>(nodes_[current_node].key_id),
                        };
                    }
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
            if constexpr (Policy::filters_candidates) {
                if (!Policy::allows_node(nodes_[current_node],
                                         candidate_groups)) {
                    return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
                }
            }
            ++consumed;
        }

        return {CANDIDATE_MISSING_OPEN_QUOTE, 0, 0};
    }

    size_t size() const noexcept { return nodes_.size(); }
    size_t max_key_len() const noexcept { return max_key_len_; }

   private:
    using Node = typename Policy::Node;

    std::vector<Node> nodes_;
    size_t max_key_len_ = 0;
};

}  // namespace teddy::detail
