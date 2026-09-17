#include "teddy/verification/group_mapping.h"

#include "teddy/compile.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace teddy::verification::detail {
namespace {

template <typename EncodeGroup>
std::vector<uint8_t> build_suffix_group_values(
    const CompilationData& compilation,
    EncodeGroup encode_group) {
    std::vector<uint8_t> suffix_group_values(compilation.suffixes.size(), 0);

    for (size_t group = 0; group < compilation.num_groups(); ++group) {
        const uint8_t group_value = encode_group(group);
        for (const uint32_t suffix_id : compilation.group_suffix_ids[group]) {
            suffix_group_values[suffix_id] = group_value;
        }
    }

    return suffix_group_values;
}

}  // namespace

std::vector<uint8_t> build_suffix_group_ids(
    const CompilationData& compilation) {
    return build_suffix_group_values(
        compilation, [](size_t group) { return static_cast<uint8_t>(group); });
}

std::vector<uint8_t> build_suffix_group_masks(
    const CompilationData& compilation) {
    return build_suffix_group_values(compilation, [](size_t group) {
        return static_cast<uint8_t>(uint8_t{1} << group);
    });
}

}  // namespace teddy::verification::detail
