#include "io/mmap_file.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

static std::string reverse_escaped_units(std::string_view text) {
    std::vector<std::string_view> tokens;
    tokens.reserve(text.size());

    for (size_t i = 0; i < text.size();) {
        if (text[i] != '\\') {
            tokens.push_back(text.substr(i, 1));
            ++i;
            continue;
        }

        if (i + 1 >= text.size()) {
            tokens.push_back(text.substr(i, 1));
            ++i;
            continue;
        }

        tokens.push_back(text.substr(i, 2));
        i += 2;
    }

    std::string reversed;
    reversed.reserve(text.size());
    for (size_t i = tokens.size(); i > 0; --i) {
        reversed.append(tokens[i - 1]);
    }

    return reversed;
}

static bool reverse_json_keys(std::string_view data,
                              const std::string& output) {
    std::ofstream ofs(output, std::ios::binary);
    if (!ofs) {
        std::cerr << "Error: Could not open file " << output
                  << " for writing.\n";
        return false;
    }

    const char* str = data.data();
    const size_t len = data.size();

    bool in_string = false;
    bool escape = false;
    size_t position = 0;
    size_t last_write = 0;

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
        while (j < len &&
               std::isspace(static_cast<unsigned char>(str[j])) != 0) {
            ++j;
        }

        if (j < len && str[j] == ':') {
            ofs.write(str + last_write,
                      static_cast<std::streamsize>(position - last_write));

            const std::string reversed = reverse_escaped_units(
                std::string_view(str + position, i - position));
            ofs.write(reversed.data(),
                      static_cast<std::streamsize>(reversed.size()));
            last_write = i;
        }

        in_string = false;
    }

    ofs.write(str + last_write, static_cast<std::streamsize>(len - last_write));
    return true;
}

static bool reverse_key_file(std::string_view data, const std::string& output) {
    std::ofstream ofs(output, std::ios::binary);
    if (!ofs) {
        std::cerr << "Error: Could not open file " << output
                  << " for writing.\n";
        return false;
    }

    size_t line_start = 0;
    while (line_start <= data.size()) {
        const size_t line_end = data.find('\n', line_start);
        const bool has_newline = (line_end != std::string_view::npos);
        const size_t end = has_newline ? line_end : data.size();

        std::string_view line = data.substr(line_start, end - line_start);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }

        const std::string reversed = reverse_escaped_units(line);
        ofs.write(reversed.data(),
                  static_cast<std::streamsize>(reversed.size()));
        if (has_newline) {
            ofs.put('\n');
            line_start = line_end + 1;
        } else {
            break;
        }
    }

    return true;
}

static void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name
              << " [--json-input <file> --json-output <file>]"
              << " [--keys-input <file> --keys-output <file>]\n";
}

int main(int argc, char* argv[]) {
    std::string json_input;
    std::string json_output;
    std::string keys_input;
    std::string keys_output;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--json-input" && i + 1 < argc) {
            json_input = argv[++i];
        } else if (arg == "--json-output" && i + 1 < argc) {
            json_output = argv[++i];
        } else if (arg == "--keys-input" && i + 1 < argc) {
            keys_input = argv[++i];
        } else if (arg == "--keys-output" && i + 1 < argc) {
            keys_output = argv[++i];
        } else {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    const bool wants_json = !json_input.empty() || !json_output.empty();
    const bool wants_keys = !keys_input.empty() || !keys_output.empty();

    if ((wants_json && (json_input.empty() || json_output.empty())) ||
        (wants_keys && (keys_input.empty() || keys_output.empty())) ||
        (!wants_json && !wants_keys)) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (wants_json) {
        MMapFile mmap(json_input.c_str());
        std::string_view data(static_cast<const char*>(mmap.data()),
                              mmap.size());
        if (!reverse_json_keys(data, json_output)) {
            return EXIT_FAILURE;
        }
    }

    if (wants_keys) {
        MMapFile mmap(keys_input.c_str());
        std::string_view data(static_cast<const char*>(mmap.data()),
                              mmap.size());
        if (!reverse_key_file(data, keys_output)) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
