#include "teddy/compile.h"

#include "core/findkey_error.h"
#include "teddy/dispatch.h"
#include "teddy/grouping.h"

#include <utility>

namespace teddy {

template <int Sigma>
static void build_compilation_tables(CompilationData& data) {
    for (int i = 0; i < FINDKEY_TEDDY_MAX_SIGMA; ++i) {
        for (int j = 0; j < 16; ++j) {
            data.low_table[i][j] = 0xFF;
            data.high_table[i][j] = 0xFF;
        }
    }

    for (int i = 0; i < Sigma; ++i) {
        for (int group = 0; group < data.num_groups; ++group) {
            bool low_filled[16] = {false};
            bool high_filled[16] = {false};

            for (uint32_t suffix_id : data.group_suffix_ids[group]) {
                const uint8_t c = data.suffixes[suffix_id][i];

                uint8_t low_nibble = c & 0x0F;
                uint8_t high_nibble = (c >> 4) & 0x0F;
                low_filled[low_nibble] = true;
                high_filled[high_nibble] = true;
            }

            const uint8_t mask = ~static_cast<uint8_t>(1u << group);
            for (int nibble = 0; nibble < 16; ++nibble) {
                if (low_filled[nibble]) {
                    data.low_table[i][nibble] &= mask;
                }
                if (high_filled[nibble]) {
                    data.high_table[i][nibble] &= mask;
                }
            }
        }
    }
}

CompilationData compile(const std::vector<std::string_view>& keys,
                        const findkey_teddy_config& config) {
    SuffixSet suffixes = prepare_suffixes(keys, config);
    return compile(std::move(suffixes), config.grouping);
}

CompilationData compile(SuffixSet suffixes,
                        findkey_teddy_grouping_config grouping_config) {
    CompilationData data{};

    if (suffixes.sigma <= 0 || suffixes.sigma > FINDKEY_TEDDY_MAX_SIGMA) {
        throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                           "Compiled Teddy suffix length is out of range");
    }
    if (suffixes.data.empty()) {
        throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                           "Teddy requires at least one suffix");
    }
    if (suffixes.end_quote_offset > 1) {
        throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                           "Invalid Teddy end quote offset");
    }

    data.sigma = suffixes.sigma;
    data.end_quote_offset = suffixes.end_quote_offset;
    data.suffixes = std::move(suffixes.data);
    data.group_suffix_ids =
        build_groups(data.suffixes, grouping_config, data.sigma);

    data.num_groups = static_cast<int>(data.group_suffix_ids.size());

    dispatch_sigma(data.sigma,
                   [&]<int Sigma>() { build_compilation_tables<Sigma>(data); });

    return data;
}

CompilationMetadata get_compilation_metadata(const CompilationData& data) {
    return {
        .sigma = data.sigma,
        .num_groups = data.num_groups,
    };
}

}  // namespace teddy
