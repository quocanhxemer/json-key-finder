#include "json_input.h"
#include "key_generation.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

bool write_keys_to_file(const std::vector<std::string>& keys,
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

void print_usage(const char* program_name) {
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

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 5 || argc > 6) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::string json_file = argv[1];
    const size_t num_keys = std::stoul(argv[2]);
    const std::string type = argv[3];
    const std::string output_file = argv[4];
    const uint32_t seed = (argc == 6)
                              ? static_cast<uint32_t>(std::stoul(argv[5]))
                              : std::random_device{}();

    const std::optional<keygen::KeyType> key_type =
        keygen::parse_key_type(type);
    if (!key_type) {
        std::cerr << "Error: Invalid type '" << type << "'.\n";
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const json_input::LoadedJson loaded_json =
        json_input::load_json_or_exit(json_file);
    const std::vector<std::string> keys_to_write = keygen::generate_keys(
        loaded_json.key_catalog(), *key_type, num_keys, seed);

    if (!write_keys_to_file(keys_to_write, output_file)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
