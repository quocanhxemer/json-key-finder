#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace keygen {

enum class KeyType {
    Highest,
    Lowest,
    NoMatch,
    AllMatch,
    Mixed,
    All,
};

struct KeyStat {
    std::string key_value;
    uint64_t frequency = 0;
};

struct KeyCatalog {
    std::vector<KeyStat> by_frequency_desc;
    std::vector<KeyStat> by_frequency_asc;
    std::unordered_set<std::string> existing_keys;
};

std::optional<KeyType> parse_key_type(std::string_view raw);

std::string_view key_type_name(KeyType key_type);

KeyCatalog build_key_catalog(std::string_view json_data);

std::vector<std::string> generate_keys(const KeyCatalog& catalog,
                                       KeyType key_type,
                                       size_t num_keys,
                                       uint32_t seed);

}  // namespace keygen
