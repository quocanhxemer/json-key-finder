#pragma once

#include "bench/bench_args.h"
#include "core/prepared_keys.h"
#include "findkey.h"

#include <cstddef>
#include <fstream>
#include <string_view>

namespace bench {

void run_bench_case(
    std::ofstream& output,
    std::string_view data,
    const KeyCase& key_case,
    const PreparedKeys& keys,
    findkey_algo algo,
    size_t repeat_count,
    size_t warmup_count,
    findkey_teddy_config teddy_config = FINDKEY_TEDDY_CONFIG_INIT);

void run_stats_case(std::ofstream& output,
                    std::string_view data,
                    const KeyCase& key_case,
                    const PreparedKeys& keys,
                    const findkey_teddy_config& teddy_config,
                    size_t repeat_count,
                    size_t warmup_count);

}  // namespace bench
