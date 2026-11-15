#pragma once

#include <string>
#include <utility>

#include "dharaninirmana/core/error_code.hpp"

namespace dharaninirmana::core {

class Error {
public:
    Error(ErrorCode code, std::string message)
        : code_(code), message_(std::move(message)) {}

    [[nodiscard]] ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }

private:
    ErrorCode code_;
    std::string message_;
};

} // namespace dharaninirmana::core
