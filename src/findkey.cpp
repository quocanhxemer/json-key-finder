#include "findkey.h"
#include "matcher_scalar.h"

#include <algorithm>
#include <string_view>

extern "C" size_t findkey_scalar(const uint8_t* data,
                                 size_t len,
                                 const uint8_t* key,
                                 size_t key_len,
                                 size_t* out_positions,
                                 size_t max_out_positions,
                                 int* out_status) {
    if (out_status) {
        *out_status = FINDKEY_OK;
    }

    if ((!data && len != 0) || (!key && key_len != 0) || !out_positions) {
        if (out_status) {
            *out_status = FINDKEY_ERR_BAD_ARGS;
        }
        return 0;
    }

    const char* str = reinterpret_cast<const char*>(data);
    const char* k = reinterpret_cast<const char*>(key);

    std::string_view data_sv(str, len);
    std::string_view key_sv(k ? k : "", key_len);

    std::vector<size_t> positions = matcher_scalar(data_sv, key_sv);

    const size_t num_positions = std::min(positions.size(), max_out_positions);

    for (size_t i = 0; i < num_positions; ++i) {
        out_positions[i] = positions[i];
    }

    return positions.size();
}
