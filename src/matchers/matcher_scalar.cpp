#include "matcher_scalar.h"

#include <cctype>

std::vector<findkey_result> matcher_scalar(
    std::string_view data,
    const teddy::HashVerifier& verifier) {
    std::vector<findkey_result> result;
    result.reserve(1024);  // rough estimate

    const char* str = data.data();
    const size_t len = data.size();

    bool in_string = false;
    bool escape = false;
    size_t key_position = 0;

    for (size_t i = 0; i < len; ++i) {
        const unsigned char c = static_cast<unsigned char>(str[i]);

        if (!in_string) {
            if (c == '"') {
                in_string = true;
                escape = false;
                key_position = i + 1;
            }
            continue;
        }

        if (escape) {
            escape = false;
            continue;
        }

        if (c == '\\') {
            escape = true;
            continue;
        }

        if (c != '"') {
            continue;
        }

        // found end of string
        size_t j = i + 1;
        while (j < len && std::isspace(static_cast<unsigned char>(str[j]))) {
            ++j;
        }

        if (j < len && str[j] == ':') {
            const std::string_view key(str + key_position, i - key_position);
            const teddy::CandidateResult candidate =
                verifier.check_key(key, key_position);
            if (candidate.type == teddy::CANDIDATE_MATCH) {
                result.push_back({candidate.position, candidate.key_id});
            }
        }
        in_string = false;
    }

    return result;
}
