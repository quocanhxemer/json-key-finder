#pragma once

#include "teddy/compile.h"
#include "teddy/verification/verifier.h"
#include "teddy/verify.h"

#include <cstddef>
#include <string_view>
#include <vector>

namespace findkey_test {

template <teddy::Verifier VerifierModel>
teddy::CandidateResult verify_candidate(
    const std::string_view data,
    const std::size_t end_quote,
    const std::vector<std::string_view>& keys,
    const uint8_t candidate_groups = 0xFF) {
    const findkey_teddy_config config = FINDKEY_TEDDY_CONFIG_INIT;
    const teddy::CompilationData compilation = teddy::compile(keys, config);
    const teddy::VerificationBuildContext context{keys, compilation};

    const VerifierModel verifier(context);
    return teddy::verify_json_key_candidate(data, end_quote, candidate_groups,
                                            verifier);
}

}  // namespace findkey_test
