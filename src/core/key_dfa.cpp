#include "core/key_dfa.h"

#include <algorithm>

DFA compile_key_dfa(const std::vector<std::string_view>& keys) {
    DFA dfa;
    dfa.nodes.emplace_back();  // root

    for (uint32_t key_id = 0; key_id < keys.size(); ++key_id) {
        const std::string_view key = keys[key_id];
        dfa.max_key_len = std::max(dfa.max_key_len, key.size());

        int32_t current_node = 0;
        for (size_t i = key.size(); i > 0; --i) {
            const uint8_t c = static_cast<uint8_t>(key[i - 1]);
            if (dfa.nodes[current_node].children[c] == -1) {
                dfa.nodes[current_node].children[c] =
                    static_cast<int32_t>(dfa.nodes.size());
                dfa.nodes.emplace_back();
            }
            current_node = dfa.nodes[current_node].children[c];
        }

        if (dfa.nodes[current_node].key_id != -1) {  // duplicated key
            continue;
        }

        dfa.nodes[current_node].key_id = key_id;
    }

    return dfa;
}

DFACompilationMetadata get_dfa_compilation_metadata(const DFA& dfa) {
    return {
        .nodes = dfa.nodes.size(),
        .max_key_len = dfa.max_key_len,
    };
}
