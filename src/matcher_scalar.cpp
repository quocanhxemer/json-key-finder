#include "matcher_scalar.h"
#include <cctype>
#include <cstring>

std::vector<size_t> matcher_scalar(std::string_view data,
                                   std::string_view key) {
    std::vector<size_t> positions;

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
        if (key_length == key.size() &&
            std::memcmp(str + position, key.data(), key_length) == 0) {
            size_t j = i + 1;
            while (j < len && isspace(static_cast<unsigned char>(str[j]))) {
                ++j;
            }
            if (j < len && str[j] == ':') {
                // only record position if followed by a colon
                positions.push_back(position);
            }
        }
        in_string = false;
    }

    return positions;
}
