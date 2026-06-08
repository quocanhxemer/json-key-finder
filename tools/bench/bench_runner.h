#pragma once

#include "bench/bench_args.h"
#include "core/prepared_keys.h"
#include "findkey.h"

#include <cstddef>
#include <fstream>
#include <optional>
#include <string_view>

namespace bench {

void run_bench_case(std::ofstream& output,
                    std::string_view data,
                    const KeyCase& key_case,
                    const PreparedKeys& keys,
                    findkey_algo algo,
                    const std::optional<TeddyConfigCase>& teddy_config_case,
                    size_t repeat_count,
                    size_t warmup_count);

void run_stats_case(std::ofstream& output,
                    std::string_view data,
                    const KeyCase& key_case,
                    const PreparedKeys& keys,
                    const TeddyConfigCase& teddy_config_case,
                    size_t repeat_count,
                    size_t warmup_count);

}  // namespace bench
