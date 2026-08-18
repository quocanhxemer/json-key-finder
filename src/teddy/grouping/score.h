#pragma once

#include "teddy/suffix.h"

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace teddy::grouping {

template <typename ScoreModel>
concept GroupingScore =
    std::default_initializable<ScoreModel> &&
    std::copy_constructible<ScoreModel> &&
    std::is_nothrow_copy_constructible_v<ScoreModel> &&
    requires(ScoreModel score, const ScoreModel other, const Suffix& suffix) {
        { score.add(suffix) } noexcept -> std::same_as<void>;
        { score.merge(other) } noexcept -> std::same_as<void>;
        { score.value() } noexcept -> std::same_as<uint64_t>;
    };

}  // namespace teddy::grouping
