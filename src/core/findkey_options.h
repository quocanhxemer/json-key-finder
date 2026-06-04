#pragma once

#include "findkey.h"

#include <optional>
#include <string_view>

namespace findkey_options {

std::optional<findkey_algo> parse_algo(std::string_view raw);

std::optional<findkey_teddy_compile_grouping_strategy> parse_grouping_strategy(
    std::string_view raw);

std::optional<findkey_teddy_compile_hash_algorithm> parse_hash_algorithm(
    std::string_view raw);

std::optional<findkey_teddy_suffix_mode> parse_suffix_mode(
    std::string_view raw);

std::optional<int> parse_sigma(std::string_view raw);

std::string_view algo_name(findkey_algo algo);

std::string_view grouping_strategy_name(
    findkey_teddy_compile_grouping_strategy strategy);

std::string_view hash_algorithm_name(
    findkey_teddy_compile_hash_algorithm algorithm);

std::string_view suffix_mode_name(findkey_teddy_suffix_mode suffix_mode);

std::string_view status_name(int status);

}  // namespace findkey_options
