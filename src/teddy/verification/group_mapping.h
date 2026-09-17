#pragma once

#include <cstdint>
#include <vector>

namespace teddy {

struct CompilationData;

namespace verification::detail {

std::vector<uint8_t> build_suffix_group_ids(const CompilationData& compilation);

std::vector<uint8_t> build_suffix_group_masks(
    const CompilationData& compilation);

}  // namespace verification::detail
}  // namespace teddy
