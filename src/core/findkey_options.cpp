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
    if (raw == "hash_std" || raw == "std_hash" || raw == "std") {
        return TEDDY_COMPILE_HASH_STD;
    }
    if (raw == "hash_adler32" || raw == "adler32_hash" || raw == "adler32") {
        return TEDDY_COMPILE_HASH_ADLER32;
    }
    if (raw == "hash_crc32" || raw == "crc32_hash" || raw == "crc32") {
        return TEDDY_COMPILE_HASH_CRC32;
    }
    if (raw == "hash_xxhash" || raw == "xxhash_hash" || raw == "xxhash") {
        return TEDDY_COMPILE_HASH_XXHASH;
    }
    if (raw == "hash_fnv1a" || raw == "fnv1a_hash" || raw == "fnv1a") {
        return TEDDY_COMPILE_HASH_FNV1A;
    }
    if (raw == "sorted_suffix_round_robin" || raw == "round_robin_suffix" ||
        raw == "sorted_round_robin") {
        return TEDDY_COMPILE_SORTED_SUFFIX_ROUND_ROBIN;
    }
    if (raw == "sorted_suffix_partition" || raw == "sort_partition" ||
        raw == "sorted_partition") {
        return TEDDY_COMPILE_SORTED_SUFFIX_PARTITION;
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
        case TEDDY_COMPILE_HASH_STD:
            return "hash_std";
        case TEDDY_COMPILE_HASH_ADLER32:
            return "hash_adler32";
        case TEDDY_COMPILE_HASH_CRC32:
            return "hash_crc32";
        case TEDDY_COMPILE_HASH_XXHASH:
            return "hash_xxhash";
        case TEDDY_COMPILE_HASH_FNV1A:
            return "hash_fnv1a";
        case TEDDY_COMPILE_SORTED_SUFFIX_ROUND_ROBIN:
            return "sorted_suffix_round_robin";
        case TEDDY_COMPILE_SORTED_SUFFIX_PARTITION:
            return "sorted_suffix_partition";
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
