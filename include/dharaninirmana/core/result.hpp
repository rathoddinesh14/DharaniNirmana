#pragma once

#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#include "dharaninirmana/core/error.hpp"

namespace dharaninirmana::core {

template <typename T>
class Result {
    static_assert(!std::is_reference_v<T>, "Result<T> cannot store a reference type.");
    static_assert(!std::is_void_v<T>, "Use the Result<void> specialization.");

public:
    [[nodiscard]] static Result success(T value) {
        return Result(std::in_place_index<0>, std::move(value));
    }

    [[nodiscard]] static Result failure(Error error) {
        return Result(std::in_place_index<1>, std::move(error));
    }

    [[nodiscard]] bool hasValue() const noexcept { return storage_.index() == 0; }
    [[nodiscard]] explicit operator bool() const noexcept { return hasValue(); }

    T& value() & {
        ensureValue();
        return std::get<0>(storage_);
    }

    const T& value() const & {
        ensureValue();
        return std::get<0>(storage_);
    }

    T&& value() && {
        ensureValue();
        return std::get<0>(std::move(storage_));
    }

    Error& error() & {
        ensureError();
        return std::get<1>(storage_);
    }

    const Error& error() const & {
        ensureError();
        return std::get<1>(storage_);
    }

    Error&& error() && {
        ensureError();
        return std::get<1>(std::move(storage_));
    }

private:
    template <std::size_t Index, typename Value>
    explicit Result(std::in_place_index_t<Index> tag, Value&& value)
        : storage_(tag, std::forward<Value>(value)) {}

    void ensureValue() const {
        if (!hasValue()) {
            throw std::logic_error("Result::value() called on a failure result");
        }
    }

    void ensureError() const {
        if (hasValue()) {
            throw std::logic_error("Result::error() called on a success result");
        }
    }

    std::variant<T, Error> storage_;
};

template <>
class Result<void> {
public:
    [[nodiscard]] static Result success() { return Result(std::nullopt); }

    [[nodiscard]] static Result failure(Error error) {
        return Result(std::optional<Error>(std::move(error)));
    }

    [[nodiscard]] bool hasValue() const noexcept { return !error_.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return hasValue(); }

    void value() const {
        if (!hasValue()) {
            throw std::logic_error("Result<void>::value() called on a failure result");
        }
    }

    Error& error() & {
        ensureError();
        return *error_;
    }

    const Error& error() const & {
        ensureError();
        return *error_;
    }

    Error&& error() && {
        ensureError();
        return std::move(*error_);
    }

private:
    explicit Result(std::optional<Error> error) : error_(std::move(error)) {}

    void ensureError() const {
        if (hasValue()) {
            throw std::logic_error("Result<void>::error() called on a success result");
        }
    }

    std::optional<Error> error_;
};

} // namespace dharaninirmana::core
