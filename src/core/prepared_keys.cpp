#include "core/prepared_keys.h"

#include <utility>

PreparedKeys prepare_keys(std::vector<std::string> keys) {
    PreparedKeys prepared;
    prepared.keys.reserve(keys.size());

    for (std::string& key : keys) {
        if (!key.empty()) {
            prepared.keys.push_back(std::move(key));
        }
    }

    prepared.ptrs.reserve(prepared.keys.size());
    prepared.lens.reserve(prepared.keys.size());
    prepared.views.reserve(prepared.keys.size());

    for (const std::string& key : prepared.keys) {
        prepared.ptrs.push_back(reinterpret_cast<const uint8_t*>(key.data()));
        prepared.lens.push_back(key.size());
        prepared.views.emplace_back(key);
    }

    return prepared;
}
