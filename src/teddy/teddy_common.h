#pragma once

#include "findkey.h"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

static constexpr int MAX_GROUPS = 8;  // must be power of 2

struct TrieNode {
    std::array<int32_t, 256> children{};
    int32_t key_id = -1;

    TrieNode() { children.fill(-1); }
};

struct DFA {
    std::vector<TrieNode> nodes;
    std::vector<uint8_t> node_group_masks;
    size_t max_key_len = 0;
};

struct TeddyCompilationData {
    int sigma = 0;
    int num_groups = 0;
    findkey_teddy_suffix_mode suffix_mode = TEDDY_SUFFIX_RAW;

    // offset from last character to the closing quote
    // 1 for RAW mode
    // 0 for QUOTED mode
    size_t end_quote_offset = 1;

    alignas(16) uint8_t low_table[FINDKEY_TEDDY_MAX_SIGMA][16] = {};
    alignas(16) uint8_t high_table[FINDKEY_TEDDY_MAX_SIGMA][16] = {};

    std::vector<std::vector<uint32_t>> group_keys;

    DFA dfa;
};

struct TeddyCompilationMetadata {
    int sigma = 0;
    int num_groups = 0;
    size_t dfa_nodes = 0;
    size_t max_key_len = 0;
    uint64_t total_score = 0;
};

TeddyCompilationData compile_teddy_data(
    const std::vector<std::string_view>& keys,
    const findkey_teddy_config& config);

TeddyCompilationMetadata get_teddy_compilation_metadata(
    const TeddyCompilationData& data);
