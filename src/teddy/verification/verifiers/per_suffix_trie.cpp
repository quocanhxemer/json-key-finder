#include "teddy/verification/verifiers/per_suffix_trie.h"

#include "core/findkey_error.h"
#include "teddy/compile.h"
#include "teddy/suffix.h"

#include <cstdint>

namespace teddy {

PerSuffixTrieVerifier::PerSuffixTrieVerifier(
    const VerificationBuildContext& context) {
    const CompilationData& teddy = context.teddy;
    sigma_ = teddy.sigma;
    end_quote_offset_ = teddy.end_quote_offset;
    suffix_tries_.resize(teddy.suffixes.size());
    suffix_trie_ids_.reserve(teddy.suffixes.size());

    for (size_t suffix_id = 0; suffix_id < teddy.suffixes.size(); ++suffix_id) {
        const uint64_t encoded =
            encode_suffix(teddy.suffixes[suffix_id].data(), sigma_);
        const bool inserted =
            suffix_trie_ids_.emplace(encoded, suffix_id).second;
        if (!inserted) {
            throw FindkeyError(
                FindkeyErrorCode::INVALID_ARGUMENT,
                "Per-suffix trie verifier requires unique suffixes");
        }
    }

    const size_t matched_key_bytes =
        static_cast<size_t>(end_quote_offset_ == 0 ? sigma_ - 1 : sigma_);

    for (uint32_t key_id = 0; key_id < context.keys.size(); ++key_id) {
        const std::string_view key = context.keys[key_id];
        if (key.size() < matched_key_bytes) {
            throw FindkeyError(
                FindkeyErrorCode::INVALID_ARGUMENT,
                "Per-suffix trie key is shorter than its compiled suffix");
        }

        const std::string_view unmatched_prefix =
            key.substr(0, key.size() - matched_key_bytes);
        suffix_tries_[teddy.key_suffix_ids[key_id]].insert(
            unmatched_prefix, key_id,
            verification::detail::PlainTriePolicy::InsertMetadata{});
    }
}

CandidateResult PerSuffixTrieVerifier::check(std::string_view input,
                                             size_t end_quote,
                                             uint8_t candidate_groups) const {
    (void)candidate_groups;

    if (sigma_ <= 0 || end_quote < end_quote_offset_) {
        return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
    }

    const size_t suffix_end = end_quote - end_quote_offset_;
    if (suffix_end + 1 < static_cast<size_t>(sigma_)) {
        return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
    }

    const size_t suffix_start = suffix_end + 1 - sigma_;
    const auto* suffix =
        reinterpret_cast<const uint8_t*>(input.data() + suffix_start);
    const uint64_t encoded = encode_suffix(suffix, sigma_);
    const auto suffix_trie = suffix_trie_ids_.find(encoded);
    if (suffix_trie == suffix_trie_ids_.end()) {
        return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
    }

    // in case suffix contains an unescaped quote, reject the candidate
    for (size_t position = suffix_start; position < end_quote; ++position) {
        if (input[position] == '"' && is_valid_quote(input.data(), position)) {
            return {CANDIDATE_KEY_NOT_FOUND, 0, 0};
        }
    }

    return suffix_tries_[suffix_trie->second].check(input, suffix_start, 0);
}

size_t PerSuffixTrieVerifier::memory_usage_bytes() const noexcept {
    size_t bytes =
        sizeof(*this) +
        verification::detail::vector_allocation_bytes(suffix_tries_) +
        verification::detail::unordered_map_allocation_bytes(suffix_trie_ids_);
    for (const Trie& trie : suffix_tries_) {
        bytes += trie.dynamic_memory_usage_bytes();
    }
    return bytes;
}

}  // namespace teddy
