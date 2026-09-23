#include "teddy/suffix.h"

#include "core/findkey_error.h"

#include <algorithm>
#include <unordered_map>

namespace teddy {
namespace {

size_t virtual_key_length(std::string_view key,
                          findkey_teddy_suffix_mode suffix_mode) {
    switch (suffix_mode) {
        case TEDDY_SUFFIX_RAW:
            return key.size();
        case TEDDY_SUFFIX_QUOTED:
            return key.size() + 1;
        default:
            throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                               "Unknown Teddy suffix mode");
    }
}

uint8_t suffix_byte(std::string_view key,
                    int sigma,
                    int suffix_index,
                    findkey_teddy_suffix_mode suffix_mode) {
    const size_t virtual_len = virtual_key_length(key, suffix_mode);
    const size_t key_index = virtual_len - sigma + suffix_index;

    if (key_index < key.size()) {
        return static_cast<uint8_t>(key[key_index]);
    }

    // for QUOTED mode, the virtual suffix byte after the last character is
    // the closing quote (")
    return '"';
}

}  // namespace

uint64_t encode_suffix(const uint8_t* suffix, int sigma) noexcept {
    uint64_t encoded = 0;
    for (int i = 0; i < sigma; ++i) {
        encoded = (encoded << 8) | suffix[i];
    }
    return encoded;
}

SuffixSet prepare_suffixes(const std::vector<std::string_view>& keys,
                           const findkey_teddy_config& config) {
    SuffixSet prepared;

    if (keys.empty()) {
        throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                           "Teddy requires at least one key");
    }
    if (config.sigma <= 0 || config.sigma > FINDKEY_TEDDY_MAX_REQUESTED_SIGMA) {
        throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                           "Requested Teddy key suffix byte count is out of "
                           "range");
    }

    if (config.suffix_mode != TEDDY_SUFFIX_RAW &&
        config.suffix_mode != TEDDY_SUFFIX_QUOTED) {
        throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                           "Unknown Teddy suffix mode");
    }

    if (std::any_of(keys.begin(), keys.end(),
                    [](std::string_view key) { return key.empty(); })) {
        throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                           "Teddy keys must not be empty");
    }

    size_t min_len = virtual_key_length(keys[0], config.suffix_mode);
    for (std::string_view key : keys) {
        min_len =
            std::min(min_len, virtual_key_length(key, config.suffix_mode));
    }

    const int target_compiled_sigma = config.suffix_mode == TEDDY_SUFFIX_QUOTED
                                          ? config.sigma + 1
                                          : config.sigma;
    prepared.sigma = std::min(static_cast<int>(min_len), target_compiled_sigma);
    prepared.end_quote_offset =
        config.suffix_mode == TEDDY_SUFFIX_QUOTED ? 0 : 1;

    prepared.data.reserve(keys.size());
    prepared.key_suffix_ids.reserve(keys.size());
    std::unordered_map<uint64_t, uint32_t> suffix_ids;
    suffix_ids.reserve(keys.size());

    for (std::string_view key : keys) {
        Suffix suffix{};
        for (int i = 0; i < prepared.sigma; ++i) {
            suffix[i] = suffix_byte(key, prepared.sigma, i, config.suffix_mode);
        }

        const uint64_t encoded = encode_suffix(suffix.data(), prepared.sigma);
        const auto [suffix_it, inserted] = suffix_ids.emplace(
            encoded, static_cast<uint32_t>(prepared.data.size()));
        if (inserted) {
            prepared.data.push_back(suffix);
        }
        prepared.key_suffix_ids.push_back(suffix_it->second);
    }

    return prepared;
}

}  // namespace teddy
