#include "key_generation.h"

#include <algorithm>
#include <cctype>
#include <random>
#include <unordered_map>

namespace keygen {
namespace {

std::unordered_map<std::string, uint64_t> get_key_frequencies(
    std::string_view data) {
    std::unordered_map<std::string, uint64_t> freq;

    const char* str = data.data();
    const size_t len = data.size();

    bool in_string = false;
    bool escape = false;
    size_t position = 0;

    for (size_t i = 0; i < len; ++i) {
        const unsigned char c = static_cast<unsigned char>(str[i]);

        if (!in_string) {
            if (c == '"') {
                in_string = true;
                escape = false;
                position = i + 1;
            }
            continue;
        }

        if (escape) {
            escape = false;
            continue;
        }

        if (c == '\\') {
            escape = true;
            continue;
        }

        if (c != '"') {
            continue;
        }

        size_t j = i + 1;
        while (j < len && std::isspace(static_cast<unsigned char>(str[j]))) {
            ++j;
        }

        if (j < len && str[j] == ':') {
            ++freq[std::string(str + position, i - position)];
        }

        in_string = false;
    }

    return freq;
}

std::vector<KeyStat> sort_keys_by_frequency(
    const std::unordered_map<std::string, uint64_t>& frequencies,
    bool ascending) {
    std::vector<KeyStat> key_stats;
    key_stats.reserve(frequencies.size());

    for (const auto& [key, frequency] : frequencies) {
        key_stats.push_back({key, frequency});
    }

    std::sort(key_stats.begin(), key_stats.end(),
              [ascending](const KeyStat& left, const KeyStat& right) {
                  if (left.frequency == right.frequency) {
                      return left.key_value < right.key_value;
                  }
                  return ascending ? left.frequency < right.frequency
                                   : left.frequency > right.frequency;
              });

    return key_stats;
}

std::vector<std::string> top_keys(const std::vector<KeyStat>& key_stats,
                                  size_t top_count) {
    std::vector<std::string> keys;
    keys.reserve(std::min(top_count, key_stats.size()));

    for (size_t i = 0; i < top_count && i < key_stats.size(); ++i) {
        keys.push_back(key_stats[i].key_value);
    }

    return keys;
}

std::vector<std::string> all_keys(const std::vector<KeyStat>& key_stats) {
    std::vector<std::string> keys;
    keys.reserve(key_stats.size());

    for (const KeyStat& key_stat : key_stats) {
        keys.push_back(key_stat.key_value);
    }

    return keys;
}

std::vector<std::string> generate_no_match_keys(
    size_t key_count,
    const std::unordered_set<std::string>& existing_keys,
    std::mt19937& rng) {
    std::vector<std::string> keys;
    keys.reserve(key_count);

    std::uniform_int_distribution<int> letter_dist(0, 25);
    for (size_t i = 0; i < key_count; ++i) {
        std::string key;
        do {
            key.clear();
            key.reserve(20);
            for (size_t j = 0; j < 16; ++j) {
                key.push_back(static_cast<char>('a' + letter_dist(rng)));
            }
            key += std::to_string(i);
        } while (existing_keys.find(key) != existing_keys.end());

        keys.push_back(std::move(key));
    }

    return keys;
}

std::vector<std::string> generate_match_keys(
    size_t key_count,
    const std::vector<KeyStat>& key_stats,
    std::mt19937& rng) {
    std::vector<std::string> keys;
    keys.reserve(key_count);

    if (key_stats.empty()) {
        return keys;
    }

    std::uniform_int_distribution<size_t> key_dist(0, key_stats.size() - 1);
    for (size_t i = 0; i < key_count; ++i) {
        keys.push_back(key_stats[key_dist(rng)].key_value);
    }

    return keys;
}

std::vector<std::string> generate_mixed_keys(
    size_t key_count,
    const std::vector<KeyStat>& key_stats,
    const std::unordered_set<std::string>& existing_keys,
    std::mt19937& rng) {
    std::vector<std::string> match_keys =
        generate_match_keys(key_count / 2, key_stats, rng);
    std::vector<std::string> no_match_keys = generate_no_match_keys(
        key_count - match_keys.size(), existing_keys, rng);

    std::vector<std::string> keys;
    keys.reserve(key_count);

    keys.insert(keys.end(), match_keys.begin(), match_keys.end());
    keys.insert(keys.end(), no_match_keys.begin(), no_match_keys.end());

    std::shuffle(keys.begin(), keys.end(), rng);
    return keys;
}

}  // namespace

std::optional<KeyType> parse_key_type(std::string_view raw) {
    if (raw == "highest") {
        return KeyType::Highest;
    }
    if (raw == "lowest") {
        return KeyType::Lowest;
    }
    if (raw == "no_match") {
        return KeyType::NoMatch;
    }
    if (raw == "all_match") {
        return KeyType::AllMatch;
    }
    if (raw == "mixed") {
        return KeyType::Mixed;
    }
    if (raw == "all") {
        return KeyType::All;
    }
    return std::nullopt;
}

std::string_view key_type_name(KeyType key_type) {
    switch (key_type) {
        case KeyType::Highest:
            return "highest";
        case KeyType::Lowest:
            return "lowest";
        case KeyType::NoMatch:
            return "no_match";
        case KeyType::AllMatch:
            return "all_match";
        case KeyType::Mixed:
            return "mixed";
        case KeyType::All:
            return "all";
    }
    return "unknown";
}

KeyCatalog build_key_catalog(std::string_view json_data) {
    auto frequencies = get_key_frequencies(json_data);
    KeyCatalog catalog;
    catalog.by_frequency_desc = sort_keys_by_frequency(frequencies, false);
    catalog.by_frequency_asc = sort_keys_by_frequency(frequencies, true);
    catalog.existing_keys.reserve(catalog.by_frequency_desc.size());

    for (const KeyStat& key_stat : catalog.by_frequency_desc) {
        catalog.existing_keys.insert(key_stat.key_value);
    }

    return catalog;
}

std::vector<std::string> generate_keys(const KeyCatalog& catalog,
                                       KeyType key_type,
                                       size_t num_keys,
                                       uint32_t seed) {
    std::mt19937 rng(seed);

    switch (key_type) {
        case KeyType::Highest:
            return top_keys(catalog.by_frequency_desc, num_keys);
        case KeyType::Lowest:
            return top_keys(catalog.by_frequency_asc, num_keys);
        case KeyType::NoMatch:
            return generate_no_match_keys(num_keys, catalog.existing_keys, rng);
        case KeyType::AllMatch:
            return generate_match_keys(num_keys, catalog.by_frequency_desc,
                                       rng);
        case KeyType::Mixed:
            return generate_mixed_keys(num_keys, catalog.by_frequency_desc,
                                       catalog.existing_keys, rng);
        case KeyType::All:
            return all_keys(catalog.by_frequency_desc);
    }

    return {};
}

}  // namespace keygen
