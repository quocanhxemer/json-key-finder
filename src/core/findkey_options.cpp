#include "core/findkey_options.h"

#include <cerrno>
#include <cstdlib>
#include <limits>
#include <string>

namespace findkey_options {

std::optional<findkey_algo> parse_algo(std::string_view raw) {
    if (raw == "scalar") {
        return SCALAR;
    }
    if (raw == "teddy") {
        return TEDDY;
    }
    if (raw == "teddy_baseline") {
        return TEDDY_BASELINE;
    }
    return std::nullopt;
}

std::optional<findkey_teddy_compile_grouping_strategy> parse_grouping_strategy(
    std::string_view raw) {
    if (raw == "paper_greedy" || raw == "greedy") {
        return TEDDY_COMPILE_PAPER_GREEDY;
    }
    if (raw == "improved_greedy") {
        return TEDDY_COMPILE_PAPER_IMPROVED_GREEDY;
    }
    if (raw == "hash") {
        return TEDDY_COMPILE_HASH;
    }
    return std::nullopt;
}

std::optional<findkey_teddy_compile_hash_algorithm> parse_hash_algorithm(
    std::string_view raw) {
    if (raw == "std") {
        return TEDDY_HASH_STD;
    }
    if (raw == "adler32") {
        return TEDDY_HASH_ADLER32;
    }
    if (raw == "crc32") {
        return TEDDY_HASH_CRC32;
    }
    if (raw == "xxhash") {
        return TEDDY_HASH_XXHASH;
    }
    if (raw == "fnv1a") {
        return TEDDY_HASH_FNV1A;
    }
    return std::nullopt;
}

std::optional<findkey_teddy_suffix_mode> parse_suffix_mode(
    std::string_view raw) {
    if (raw == "raw") {
        return TEDDY_SUFFIX_RAW;
    }
    if (raw == "quote-suffix") {
        return TEDDY_SUFFIX_QUOTED;
    }
    return std::nullopt;
}

std::optional<int> parse_sigma(std::string_view raw) {
    if (raw.empty()) {
        return std::nullopt;
    }

    const std::string text(raw);
    char* end = nullptr;
    errno = 0;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || *end != '\0') {
        return std::nullopt;
    }

    if (value <= 0 || value > FINDKEY_TEDDY_MAX_SUFFIX_LENGTH ||
        value > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }

    return static_cast<int>(value);
}

std::string_view algo_name(findkey_algo algo) {
    switch (algo) {
        case SCALAR:
            return "scalar";
        case TEDDY:
            return "teddy";
        case TEDDY_BASELINE:
            return "teddy_baseline";
        default:
            return "unknown";
    }
}

std::string_view grouping_strategy_name(
    findkey_teddy_compile_grouping_strategy strategy) {
    switch (strategy) {
        case TEDDY_COMPILE_PAPER_GREEDY:
            return "paper_greedy";
        case TEDDY_COMPILE_PAPER_IMPROVED_GREEDY:
            return "improved_greedy";
        case TEDDY_COMPILE_HASH:
            return "hash";
        default:
            return "unknown";
    }
}

std::string_view hash_algorithm_name(
    findkey_teddy_compile_hash_algorithm algorithm) {
    switch (algorithm) {
        case TEDDY_HASH_STD:
            return "std";
        case TEDDY_HASH_ADLER32:
            return "adler32";
        case TEDDY_HASH_CRC32:
            return "crc32";
        case TEDDY_HASH_XXHASH:
            return "xxhash";
        case TEDDY_HASH_FNV1A:
            return "fnv1a";
        default:
            return "unknown";
    }
}

std::string_view suffix_mode_name(findkey_teddy_suffix_mode suffix_mode) {
    switch (suffix_mode) {
        case TEDDY_SUFFIX_RAW:
            return "raw";
        case TEDDY_SUFFIX_QUOTED:
            return "quote-suffix";
        default:
            return "unknown";
    }
}

std::string_view status_name(int status) {
    switch (status) {
        case FINDKEY_OK:
            return "ok";
        case FINDKEY_ERR_BAD_ARGS:
            return "bad_args";
        case FINDKEY_TEDDY_NOT_SUPPORTED:
            return "teddy_not_supported";
        case FINDKEY_ERR_UNKNOWN_ALGO:
            return "unknown_algo";
        default:
            return "unknown_status";
    }
}

}  // namespace findkey_options
