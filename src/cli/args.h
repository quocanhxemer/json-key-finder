#pragma once

#include "findkey.h"

struct ParsedCliArgs {
    findkey_algo algo = SCALAR;
    findkey_teddy_config teddy_config = FINDKEY_TEDDY_CONFIG_INIT;
    const char* keys_path = nullptr;
    const char* data_path = nullptr;
    bool collect_stats = false;
    bool print_positions = false;
};

ParsedCliArgs parse_cli_args_or_exit(int argc, char** argv);
