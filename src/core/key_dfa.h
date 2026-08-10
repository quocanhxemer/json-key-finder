#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

struct TrieNode {
    std::array<int32_t, 256> children{};
    int32_t key_id = -1;

    TrieNode() { children.fill(-1); }
};

struct DFA {
    std::vector<TrieNode> nodes;
    size_t max_key_len = 0;
};

struct DFACompilationMetadata {
    size_t nodes = 0;
    size_t max_key_len = 0;
};

DFA compile_key_dfa(const std::vector<std::string_view>& keys);

DFACompilationMetadata get_dfa_compilation_metadata(const DFA& dfa);
