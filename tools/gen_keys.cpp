#include "io/mmap_file.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

static std::unordered_map<std::string, uint64_t> get_key_frequencies(
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

        const size_t key_length = i - position;

        size_t j = i + 1;
        while (j < len && isspace(static_cast<unsigned char>(str[j]))) {
            ++j;
        }

        if (j < len && str[j] == ':') {
            const std::string key(str + position, key_length);
            ++freq[key];
        }

        in_string = false;
    }

    return freq;
}

struct KeyStat {
    std::string key_value;
    uint64_t frequency;
};

static std::vector<KeyStat> sort_keys_by_frequency(
    const std::unordered_map<std::string, uint64_t>& freq,
    bool ascending = false) {
    std::vector<KeyStat> key_stats;
    key_stats.reserve(freq.size());

    for (const auto& [key, frequency] : freq) {
        key_stats.push_back({key, frequency});
    }

    std::sort(key_stats.begin(), key_stats.end(),
              [&ascending](const KeyStat& a, const KeyStat& b) {
                  // alphabetical order for keys with the same frequency
                  if (a.frequency == b.frequency) {
                      return a.key_value < b.key_value;
                  }
                  return ascending ? (a.frequency < b.frequency)
                                   : (a.frequency > b.frequency);
              });

    return key_stats;
}

static std::vector<std::string> top_keys(const std::vector<KeyStat>& key_stats,
                                         size_t top_n) {
    std::vector<std::string> top_keys;
    top_keys.reserve(std::min(top_n, key_stats.size()));

    for (size_t i = 0; i < top_n && i < key_stats.size(); ++i) {
        top_keys.push_back(key_stats[i].key_value);
    }

    return top_keys;
}

static std::vector<std::string> generate_no_match_keys(
    size_t n,
    const std::unordered_set<std::string>& existing_keys,
    std::mt19937& rng) {
    std::vector<std::string> no_match_keys;
    no_match_keys.reserve(n);

    std::uniform_int_distribution<int> dist(0, 25);
    for (size_t i = 0; i < n; ++i) {
        std::string key;
        do {
            key.clear();
            key.reserve(20);
            for (size_t j = 0; j < 16; ++j) {
                key.push_back('a' + dist(rng));
            }
            key += std::to_string(i);
        } while (existing_keys.find(key) != existing_keys.end());
        no_match_keys.emplace_back(key);
    }

    return no_match_keys;
}

static std::vector<std::string> generate_match_keys(
    size_t n,
    const std::vector<KeyStat>& stats,
    std::mt19937& rng) {
    std::vector<std::string> match_keys;
    match_keys.reserve(n);

    if (stats.empty()) {
        return match_keys;
    }

    std::uniform_int_distribution<size_t> dist(0, stats.size() - 1);
    for (size_t i = 0; i < n; ++i) {
        const size_t index = dist(rng);
        match_keys.push_back(stats[index].key_value);
    }

    return match_keys;
}

static std::vector<std::string> generate_mix_keys(
    size_t n,
    const std::vector<KeyStat>& stats,
    const std::unordered_set<std::string>& existing_keys,
    std::mt19937& rng) {
    std::vector<std::string> mix_keys;
    mix_keys.reserve(n);

    std::vector<std::string> match_keys =
        generate_match_keys(n / 2, stats, rng);
    std::vector<std::string> no_match_keys =
        generate_no_match_keys(n - match_keys.size(), existing_keys, rng);

    mix_keys.insert(mix_keys.end(), match_keys.begin(), match_keys.end());
    mix_keys.insert(mix_keys.end(), no_match_keys.begin(), no_match_keys.end());

    std::shuffle(mix_keys.begin(), mix_keys.end(), rng);

    return mix_keys;
}

static bool write_keys_to_file(const std::vector<std::string>& keys,
                               const std::string& filename) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Error: Could not open file " << filename
                  << " for writing.\n";
        return false;
    }

    for (const std::string& key : keys) {
        ofs << key << '\n';
    }

    return true;
}

static std::vector<std::string> all_keys(const std::vector<KeyStat>& stats) {
    std::vector<std::string> all_keys;
    all_keys.reserve(stats.size());

    for (const auto& stat : stats) {
        all_keys.push_back(stat.key_value);
    }

    return all_keys;
}

static void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name
              << " <json_file> <num_keys> <type> <output_file> [seed]\n"
              << "Types:\n"
              << "  highest\n"
              << "  lowest\n"
              << "  no_match\n"
              << "  all_match\n"
              << "  mixed\n"
              << "  all (writes all keys, num_keys ignored)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 5 || argc > 6) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::string json_file = argv[1];
    const size_t num_keys = std::stoul(argv[2]);
    const std::string type = argv[3];
    const std::string output_file = argv[4];
    uint64_t seed = (argc == 6) ? std::stoul(argv[5]) : std::random_device{}();

    MMapFile mmap(json_file.c_str());
    const char* data = static_cast<const char*>(mmap.data());
    size_t size = mmap.size();

    if (size == 0) {
        std::cerr << "Error: File is empty.\n";
        return EXIT_FAILURE;
    }

    std::string_view json_data(data, size);
    auto freq = get_key_frequencies(json_data);
    auto key_stats = sort_keys_by_frequency(freq);
    auto key_stats_lowest = sort_keys_by_frequency(freq, true);

    std::unordered_set<std::string> existing_keys;
    for (const auto& stat : key_stats) {
        existing_keys.insert(stat.key_value);
    }

    std::vector<std::string> keys_to_write;

    std::mt19937 rng(seed);
    if (type == "highest") {
        keys_to_write = top_keys(key_stats, num_keys);
    } else if (type == "lowest") {
        keys_to_write = top_keys(key_stats_lowest, num_keys);
    } else if (type == "no_match") {
        keys_to_write = generate_no_match_keys(num_keys, existing_keys, rng);
    } else if (type == "all_match") {
        keys_to_write = generate_match_keys(num_keys, key_stats, rng);
    } else if (type == "mixed") {
        keys_to_write =
            generate_mix_keys(num_keys, key_stats, existing_keys, rng);
    } else if (type == "all") {
        keys_to_write = all_keys(key_stats);
    } else {
        std::cerr << "Error: Invalid type '" << type << "'.\n";
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (!write_keys_to_file(keys_to_write, output_file)) {
        return EXIT_FAILURE;
    }

    return 0;
}
