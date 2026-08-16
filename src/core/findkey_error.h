#pragma once

#include <stdexcept>
#include <string>
#include <utility>

enum class FindkeyErrorCode {
    INVALID_ARGUMENT,
    NOT_SUPPORTED,
    UNKNOWN_ALGORITHM,
};

class FindkeyError : public std::runtime_error {
   public:
    FindkeyError(FindkeyErrorCode code, std::string message)
        : std::runtime_error(std::move(message)), code_(code) {}

    FindkeyErrorCode code() const noexcept { return code_; }

   private:
    FindkeyErrorCode code_;
};
