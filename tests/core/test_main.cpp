#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "dharaninirmana/core/error.hpp"
#include "dharaninirmana/core/error_code.hpp"
#include "dharaninirmana/core/result.hpp"

namespace {

int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        std::cerr << "[FAIL] " << description << '\n';
        ++failures;
    } else {
        std::cout << "[PASS] " << description << '\n';
    }
}

template <typename Function>
void checkThrowsLogicError(Function&& function, const char* description) {
    try {
        std::forward<Function>(function)();
        check(false, description);
    } catch (const std::logic_error&) {
        check(true, description);
    } catch (...) {
        check(false, description);
    }
}

} // namespace

int main() {
    using dharaninirmana::core::Error;
    using dharaninirmana::core::ErrorCode;
    using dharaninirmana::core::Result;

    const auto success = Result<int>::success(42);
    check(success.hasValue(), "value result reports success");
    check(static_cast<bool>(success), "value result converts explicitly to true");
    check(success.value() == 42, "successful value can be read");

    auto failure = Result<int>::failure(Error{ErrorCode::InvalidArgument, "bad input"});
    check(!failure.hasValue(), "failure result reports failure");
    check(failure.error().code() == ErrorCode::InvalidArgument, "error code is preserved");
    check(failure.error().message() == "bad input", "error message is preserved");
    checkThrowsLogicError([&] { (void)failure.value(); }, "value access on failure throws logic_error");
    checkThrowsLogicError([&] { (void)success.error(); }, "error access on success throws logic_error");

    auto moveOnly = Result<std::unique_ptr<int>>::success(std::make_unique<int>(7));
    static_assert(!std::is_copy_constructible_v<Result<std::unique_ptr<int>>>);
    check(*std::move(moveOnly).value() == 7, "Result supports move-only values and rvalue extraction");

    const auto voidSuccess = Result<void>::success();
    check(voidSuccess.hasValue(), "void result reports success");
    voidSuccess.value();
    check(true, "successful void result can be checked");

    auto voidFailure = Result<void>::failure(Error{ErrorCode::InvalidState, "not ready"});
    check(!voidFailure.hasValue(), "failed void result reports failure");
    check(voidFailure.error().message() == "not ready", "void result preserves error");
    checkThrowsLogicError([&] { voidFailure.value(); }, "void value access on failure throws logic_error");
    checkThrowsLogicError([&] { (void)voidSuccess.error(); }, "void error access on success throws logic_error");

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All Core tests passed.\n";
    return EXIT_SUCCESS;
}
