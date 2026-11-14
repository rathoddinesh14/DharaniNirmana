
# Core API

## 1. Purpose

The Core module provides foundational types and utilities used across DharaniNirmana.

It must remain small, stable, graphics-independent, and free from domain-specific terrain logic.

Core provides:

- Error representation.
- Result-based error handling.
- Fundamental mathematical types.
- Common validation conventions.

Core must not depend on any other DharaniNirmana module.

## 2. Namespace and Header Organization

Namespace:

```cpp
dharaninirmana::core
```

Public headers:

```text
include/dharaninirmana/core/
├── error.hpp
├── result.hpp
└── vector.hpp
```

## 3. Error Model

### 3.1 ErrorCode

The initial error categories are:

| Error Code | Meaning |
|---|---|
| InvalidArgument | An input argument is invalid |
| InvalidState | An object is not in a valid state |
| OutOfRange | An index or value is outside the permitted range |
| InvalidData | Input or internal data violates an invariant |
| UnsupportedOperation | The requested operation is unsupported |
| ResourceExhausted | Required memory or another resource is unavailable |
| InternalError | An unexpected internal failure occurred |

Module-specific error codes may be introduced later when generic categories are insufficient.

### 3.2 Error Structure

Conceptual structure:

```cpp
struct Error {
    ErrorCode code;
    std::string message;
};
```

Requirements:

- `code` identifies the failure category.
- `message` provides diagnostic context.
- Messages must not be used for programmatic error classification.
- Error objects own their messages.
- Error objects must not depend on external object lifetimes.

## 4. Result Type

### 4.1 Design Decision

DharaniNirmana targets C++20.

`std::expected` is a C++23 facility and cannot be assumed.

DharaniNirmana will provide its own result abstraction implemented using:

```cpp
std::variant<T, Error>
```

The implementation will be header-only.

No external dependency is required.

### 4.2 Result Semantics

A `Result<T>` contains exactly one of:

- A successful value of type `T`.
- An Error describing the failure.

The result must never represent success and failure simultaneously.

The result owns its contained value or error.

### 4.3 Construction Policy

Construction must be explicit.

The public API should use named factories:

```cpp
Result<T>::success(value)

Result<T>::failure(error)
```

Direct implicit construction from `T` or `Error` must not be supported.

This prevents accidental ambiguity and makes success and failure visible at call sites.

The implementation should use indexed variant construction internally so that `T` cannot be confused with the Error alternative.

### 4.4 Public Interface

The conceptual interface is:

```cpp
template <typename T>
class Result {
public:
    static Result success(T value);
    static Result failure(Error error);

    bool hasValue() const noexcept;

    explicit operator bool() const noexcept;

    T& value() &;
    const T& value() const &;
    T&& value() &&;

    Error& error() &;
    const Error& error() const &;
};
```

The implementation must support move-only value types.

Copy operations should be available only when the contained type supports copying.

Move operations should follow the capabilities of the contained type.

Reference types are not valid Result value types.

`Result<void>` must be implemented as a dedicated specialization.

### 4.5 Accessor Behavior

The following rules apply:

- `value()` is valid only when the result contains a value.
- `error()` is valid only when the result contains an error.
- Invalid accessor usage is a programming error.
- Invalid accessor usage must not silently return default or invalid data.

The initial implementation will use `std::get` internally.

Consequently, accessing the wrong alternative produces `std::bad_variant_access`.

This provides predictable behavior using a standard C++ mechanism.

The API does not treat invalid accessor usage as an ordinary operation failure.

### 4.6 Result<void>

Operations that do not produce a value use:

```cpp
Result<void>
```

Its conceptual implementation uses:

```cpp
std::optional<Error>
```

Semantics:

- An empty optional represents success.
- A populated optional represents failure.

Public factories:

```cpp
Result<void>::success()

Result<void>::failure(error)
```

Public access:

```cpp
bool hasValue() const noexcept;

explicit operator bool() const noexcept;

const Error& error() const;
```

The `value()` accessor is unnecessary for `Result<void>`.

Calling `error()` on a successful result is a programming error and must produce a predictable diagnostic.

### 4.7 Usage Example

```cpp
Result<HeightField> result =
    generator.generate(parameters);

if (!result) {
    reportError(result.error());
    return;
}

HeightField terrain = std::move(result).value();
```

For operations without a return value:

```cpp
Result<void> result = validateMesh(mesh);

if (!result) {
    reportError(result.error());
    return;
}
```

## 5. Error Propagation

Components should preserve useful error information when propagating failures.

Requirements:

- Preserve the original error category where possible.
- Add diagnostic context when it improves troubleshooting.
- Do not silently discard failed results.
- Do not replace meaningful errors with generic messages.
- Do not expose partially initialized output objects.

Routine validation failures should use Result rather than exceptions.

Exceptions may still occur for exceptional runtime conditions or programming errors.

## 6. Mathematical Primitives

### 6.1 Design Goals

Core provides basic mathematical primitives required by terrain and geometry calculations.

These types must not depend on graphics APIs or external mathematics libraries.

### 6.2 Vec2d

Conceptual structure:

```cpp
struct Vec2d {
    double x;
    double y;
};
```

Used for generic two-dimensional mathematical operations.

It must not be used to represent terrain-specific coordinate semantics when a dedicated domain type is clearer.

### 6.3 Vec3d

Conceptual structure:

```cpp
struct Vec3d {
    double x;
    double y;
    double z;
};
```

Used for:

- Three-dimensional positions.
- Geometric calculations.
- Surface normals.
- Spatial calculations.

### 6.4 Precision Policy

Core mathematical primitives use double precision.

Rendering adapters may convert values to float when required by graphics APIs.

### 6.5 Required Operations

Initial operations:

- Addition.
- Subtraction.
- Scalar multiplication.
- Scalar division.
- Dot product.
- Cross product for Vec3d.
- Vector length.
- Normalization.

Requirements:

- Division by zero must be handled explicitly.
- Zero-length normalization must not silently produce invalid values.
- Non-finite results must be handled according to the operation contract.
- Tolerance-based comparisons must use explicitly selected tolerances.

## 7. Numerical Conventions

All public numerical APIs must document:

- Expected units.
- Valid ranges.
- Precision expectations.
- Tolerance requirements.
- Behavior for non-finite values.

Initial conventions:

| Concept | Convention |
|---|---|
| Internal coordinates | Double precision |
| Elevation | Double precision |
| Horizontal spacing | Double precision |
| Mesh positions | Double precision |
| Renderer positions | Converted when required |
| Angles | Radians |
| Length | Application-defined world units |

DharaniNirmana does not impose a physical unit system.

Applications must use consistent units within a terrain dataset.

## 8. Validation Utilities

Core may provide small reusable validation utilities.

Initial requirements include:

- Finite floating-point validation.
- Positive numeric validation.
- Valid numeric range checks.
- Safe dimension calculations.
- Non-zero vector magnitude checks.

Domain-specific validation belongs in the corresponding module.

## 9. Memory and Ownership

Core types follow normal C++ value semantics.

- Error owns its diagnostic message.
- Result owns its contained value or error.
- Mathematical primitives own no dynamic resources.
- No global mutable error state is permitted.
- No raw owning pointers are permitted.

## 10. Testing Requirements

### Error Tests

- Error code correctness.
- Message ownership.
- Copy and move behavior.

### Result Tests

- Successful construction.
- Failed construction.
- Boolean conversion.
- Value access.
- Error access.
- Result<void> behavior.
- Move-only value support.
- Copy behavior where supported.
- Invalid accessor behavior.
- Construction constraints.

### Vector Tests

- Arithmetic operations.
- Dot product.
- Cross product.
- Vector length.
- Normalization.
- Zero-length normalization.
- Non-finite numerical cases.

Core tests must not require a graphics context.

## 11. Design Constraints

Core must not:

- Depend on Vishwakarma.
- Depend on OpenGL, GLFW, or Vulkan.
- Depend on terrain-specific structures.
- Introduce global mutable state.
- Introduce unnecessary external dependencies.
- Introduce a comprehensive mathematics framework without a concrete requirement.

## 12. Completion Criteria

The Core API is ready for implementation when:

- Error categories are defined.
- Result<T> semantics are specified.
- Result<void> semantics are specified.
- Construction is explicit.
- Accessor behavior is defined.
- Mathematical primitives are documented.
- Numerical conventions are established.
- Ownership rules are explicit.
- Independent tests are identified.
