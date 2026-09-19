#include "teddy/verification/verifiers/hash.h"

#include <algorithm>

namespace teddy {

HashVerifier::HashVerifier(const std::vector<std::string_view>& keys) {
    keys_.reserve(keys.size());

    for (uint32_t key_id = 0; key_id < keys.size(); ++key_id) {
        max_key_len_ = std::max(max_key_len_, keys[key_id].size());
        keys_.emplace(keys[key_id], key_id);
    }
}

HashVerifier::HashVerifier(const VerificationBuildContext& context)
    : HashVerifier(context.keys) {}

}  // namespace teddy
