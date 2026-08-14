#include "teddy/teddy_suffix.h"

#include <algorithm>
#include <unordered_set>

namespace {

size_t virtual_key_length(std::string_view key,
                          findkey_teddy_suffix_mode suffix_mode) {
    switch (suffix_mode) {
        case TEDDY_SUFFIX_RAW:
            return key.size();
        case TEDDY_SUFFIX_QUOTED:
            return key.size() + 1;
        default:
            return 0;
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

uint64_t encode_suffix(const TeddySuffix& suffix, int sigma) {
    uint64_t encoded = 0;
    for (int i = 0; i < sigma; ++i) {
        encoded = (encoded << 8) | suffix[i];
    }
    return encoded;
}

}  // namespace

TeddySuffixSet prepare_teddy_suffixes(const std::vector<std::string_view>& keys,
                                      const findkey_teddy_config& config) {
    TeddySuffixSet prepared;

    if (keys.empty() || config.sigma <= 0 ||
        config.sigma > FINDKEY_TEDDY_MAX_SUFFIX_LENGTH) {
        return prepared;
    }

    if (config.suffix_mode != TEDDY_SUFFIX_RAW &&
        config.suffix_mode != TEDDY_SUFFIX_QUOTED) {
        return prepared;
    }

    size_t min_len = virtual_key_length(keys[0], config.suffix_mode);
    for (std::string_view key : keys) {
        min_len =
            std::min(min_len, virtual_key_length(key, config.suffix_mode));
    }

    const int requested_sigma = config.suffix_mode == TEDDY_SUFFIX_QUOTED
                                    ? config.sigma + 1
                                    : config.sigma;
    prepared.sigma = std::min(static_cast<int>(min_len), requested_sigma);
    prepared.end_quote_offset =
        config.suffix_mode == TEDDY_SUFFIX_QUOTED ? 0 : 1;

    if (prepared.sigma <= 0) {
        return {};
    }

    prepared.data.reserve(keys.size());
    std::unordered_set<uint64_t> seen;
    seen.reserve(keys.size());

    for (std::string_view key : keys) {
        TeddySuffix suffix{};
        for (int i = 0; i < prepared.sigma; ++i) {
            suffix[i] = suffix_byte(key, prepared.sigma, i, config.suffix_mode);
        }

        if (seen.insert(encode_suffix(suffix, prepared.sigma)).second) {
            prepared.data.push_back(suffix);
        }
    }

    return prepared;
}
