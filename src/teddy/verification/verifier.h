#pragma once

#include "findkey.h"
#include "teddy/verification/result.h"

#include <concepts>
#include <cstddef>
#include <string_view>
#include <vector>

namespace teddy {

struct CompilationData;

struct VerificationBuildContext {
    const std::vector<std::string_view>& keys;
    const CompilationData& teddy;
};

template <typename VerifierModel>
concept Verifier =
    std::constructible_from<VerifierModel, const VerificationBuildContext&> &&
    requires(const VerifierModel& verifier,
             std::string_view input,
             size_t end_quote,
             uint8_t candidate_groups) {
        {
            VerifierModel::strategy
        } -> std::convertible_to<findkey_teddy_verification_strategy>;
        {
            verifier.check(input, end_quote, candidate_groups)
        } -> std::same_as<CandidateResult>;
        { verifier.size() } noexcept -> std::same_as<size_t>;
    };

}  // namespace teddy
