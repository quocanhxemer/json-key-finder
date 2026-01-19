#include "matcher_scalar.h"
#include <cctype>
#include <cstring>
#include <unordered_map>

std::vector<findkey_result> matcher_scalar(
    std::string_view data,
    const std::vector<std::string_view>& keys) {
    std::vector<findkey_result> result;
    result.reserve(1024);  // rough estimate

    std::unordered_map<std::string_view, uint32_t> key_map;
    key_map.reserve(keys.size());

    for (uint32_t i = 0; i < keys.size(); ++i) {
        if (key_map.find(keys[i]) == key_map.end()) {
            key_map[keys[i]] = i;
        }
    }

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

        // found end of string
        const size_t key_length = i - position;
        size_t j = i + 1;
        while (j < len && isspace(static_cast<unsigned char>(str[j]))) {
            ++j;
        }

        if (j < len && str[j] == ':') {
            std::string_view sv(str + position, key_length);
            auto it = key_map.find(sv);
            if (it != key_map.end()) {
                result.push_back(findkey_result{position, it->second});
            }
        }
        in_string = false;
    }

    return result;
}
