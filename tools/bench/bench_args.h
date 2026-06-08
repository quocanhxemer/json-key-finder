#pragma once

#include "findkey.h"
#include "key_generation.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace bench {

enum class RunMode {
    Bench,
    Stats,
    Both,
};

struct KeyCase {
    std::string json_path;
    keygen::KeyType key_type = keygen::KeyType::Highest;
    size_t num_keys = 0;
    uint32_t seed = 0;
};

struct TeddyConfigCase {
    findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
};

struct Options {
    RunMode mode = RunMode::Both;
    std::vector<std::string> json_paths;
    std::vector<keygen::KeyType> key_types;
    std::vector<size_t> num_keys;
    std::vector<uint32_t> seeds;
    std::vector<findkey_algo> algos;
    std::vector<findkey_teddy_compile_grouping_strategy> grouping_strategies;
    std::vector<findkey_teddy_compile_hash_algorithm> hash_algorithms;
    std::vector<findkey_teddy_suffix_mode> suffix_modes;
    std::vector<int> sigmas;
    size_t repeats = 5;
    size_t warmup = 1;
    std::filesystem::path out_dir = "bench_out_cpp";
    bool dry_run = false;
};

Options parse_options(int argc, char** argv);

bool mode_includes_bench(RunMode mode);

bool mode_includes_stats(RunMode mode);

std::vector<TeddyConfigCase> make_teddy_configs(const Options& options);

std::vector<KeyCase> make_key_cases(const Options& options);

void print_dry_run(const Options& options,
                   const std::vector<KeyCase>& key_cases,
                   const std::vector<TeddyConfigCase>& teddy_configs);

}  // namespace bench
