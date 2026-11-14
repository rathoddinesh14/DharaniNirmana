
# Core API

## 1. Purpose

The Core module provides foundational types and abstractions shared across DharaniNirmana.

It is the lowest-level module in the architecture and must remain independent of all other DharaniNirmana modules.

Core provides:

- Mathematical vector types.
- Error representation.
- Explicit result types.
- Shared low-level utilities.

The Core API must be lightweight, predictable, and suitable for use throughout the engine.

## 2. Design Principles

The Core API follows these principles:

- **Minimal dependencies:** Depend only on the C++ standard library.
- **Explicit errors:** Operations that may fail communicate failure through Result types.
- **Value semantics:** Mathematical types should behave as ordinary value types.
- **Const correctness:** Read-only operations must not modify their operands.
- **Numerical awareness:** Mathematical operations must document their behavior for invalid or unrepresentable values.
- **Move support:** Result types must support move-only values.
- **Clear contracts:** Public interfaces must define ownership, validity, and failure behavior.

## 3. Namespace

All public Core types belong to:

```cpp
namespace dharaninirmana::core {
}
```

The namespace must remain consistent across all public headers and implementation files.

## 4. Header and File Organization

The initial Core files are organized as follows:

```text
.
├── include/dharaninirmana/core/
│   ├── result.hpp
│   ├── error.hpp
│   ├── error_code.hpp
│   ├── vec2d.hpp
│   └── vec3d.hpp
├── src/core/
│   └── [private implementation files, if required]
└── tests/core/
    └── [Core API tests]
```

Public headers define the Core API. Exact implementation and test filenames may be determined as implementation is organized.

## 5. Mathematical Types

### 5.1 Vec2d

`Vec2d` represents a two-dimensional vector using double-precision floating-point components.

Conceptual declaration:

```cpp
namespace dharaninirmana::core {

struct Vec2d {
    double x{};
    double y{};
};

}
```

The type provides value semantics.

It may support common operations such as:

- Addition.
- Subtraction.
- Scalar multiplication.
- Scalar division.
- Equality comparison.
- Dot product.
- Length calculation.
- Normalization.

The implementation must document the behavior of operations involving non-finite values and zero-length vectors.

### 5.2 Vec3d

`Vec3d` represents a three-dimensional vector using double-precision floating-point components.

Conceptual declaration:

```cpp
namespace dharaninirmana::core {

struct Vec3d {
    double x{};
    double y{};
    double z{};
};

}
```

The type may support:

- Addition.
- Subtraction.
- Scalar multiplication.
- Scalar division.
- Equality comparison.
- Dot product.
- Cross product.
- Length calculation.
- Normalization.

### 5.3 Coordinate Convention

DharaniNirmana uses the following coordinate convention:

| Component | Meaning |
|---|---|
| X | Horizontal axis |
| Y | Elevation axis |
| Z | Horizontal axis |

The vector types themselves remain generic mathematical types.

They must not encode terrain-specific behavior.

### 5.4 Numerical Behavior

Vector operations must account for floating-point limitations.

In particular:

- Components may contain non-finite values unless an API explicitly requires finite vectors.
- Length calculations should use numerically stable algorithms such as `std::hypot`.
- Squared length calculations may overflow for large finite components.
- Normalization must handle zero-length vectors explicitly.
- Division by zero must not silently produce an apparently valid result.

Core mathematical operations should not silently clamp or modify invalid inputs.

Where an operation can fail, its failure behavior must be documented.

## 6. Error Representation

### 6.1 Purpose

The `Error` type represents a failure detected by a public DharaniNirmana operation.

It provides a common error representation across modules.

### 6.2 Conceptual Interface

```cpp
namespace dharaninirmana::core {

class Error {
public:
    // Error construction and inspection.
};

}
```

The exact representation is an implementation detail.

The public interface should provide sufficient information to understand the failure.

### 6.3 Error Information

An Error should support:

- An error category or code.
- A human-readable diagnostic message.
- Optional contextual information where useful.

Error categories should be stable enough for programmatic inspection.

Diagnostic messages may evolve as implementations improve.

### 6.4 Error Ownership

An Error owns its diagnostic information.

It must not retain references to temporary strings or caller-owned buffers.

Copying and moving an Error must preserve its diagnostic information.

## 7. Result Type

### 7.1 Purpose

`Result<T>` represents the outcome of an operation that may either succeed with a value or fail with an Error.

It avoids using exceptions as the normal mechanism for reporting expected operational failures.

The type is based on:

```cpp
std::variant<T, Error>
```

### 7.2 Conceptual Interface

```cpp
namespace dharaninirmana::core {

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
    Error&& error() &&;

private:
    std::variant<T, Error> storage_;
};

}
```

This declaration illustrates the intended contract.

The implementation may adjust constructors, constraints, and overloads while preserving the documented semantics.

### 7.3 Explicit Construction

Results must be created through explicit factories:

```cpp
auto result =
    Result<HeightField>::success(std::move(heightField));
```

Failure:

```cpp
auto result =
    Result<HeightField>::failure(error);
```

Implicit conversion from a value or Error must not make it possible to construct an ambiguous result.

### 7.4 State Inspection

`hasValue()` returns true when the result contains a value.

It returns false when the result contains an Error.

The boolean conversion provides the same information:

```cpp
if (result) {
    // Success
} else {
    // Failure
}
```

### 7.5 Value Access

Calling `value()` when the result contains an Error is a programming error.

The implementation must report this misuse explicitly.

The chosen access mechanism must have documented behavior, such as throwing `std::logic_error`.

The implementation must not return an uninitialized or fabricated value.

Value access should provide appropriate lvalue, const-lvalue, and rvalue overloads.

### 7.6 Error Access

Calling `error()` when the result contains a value is a programming error.

The implementation must report this misuse explicitly, such as through `std::logic_error`.

The implementation must not fabricate an Error or return invalid storage.

Appropriate lvalue, const-lvalue, and rvalue overloads should be provided.

## 8. Move-Only Value Support

`Result<T>` must support move-only types.

Examples include:

- Types containing `std::unique_ptr`.
- Resource-owning value types.
- Types with deleted copy constructors.

The implementation must not require `T` to be copy-constructible merely to instantiate or use a successful Result.

The success factory must support moving a value into the Result.

Value access through an rvalue Result must permit moving the contained value out.

The implementation should use appropriate C++20 type constraints to prevent invalid operations from being instantiated.

## 9. Result<void>

### 9.1 Purpose

Some operations report success or failure without producing a value.

These operations use `Result<void>`.

### 9.2 Representation

The specialization uses:

```cpp
std::optional<Error>
```

An empty optional represents success.

A populated optional represents failure.

### 9.3 Conceptual Interface

```cpp
namespace dharaninirmana::core {

template <>
class Result<void> {
public:
    static Result success();
    static Result failure(Error error);

    bool hasValue() const noexcept;
    explicit operator bool() const noexcept;

    void value() const;

    Error& error() &;
    const Error& error() const &;
    Error&& error() &&;

private:
    std::optional<Error> error_;
};

}
```

### 9.4 Success and Failure

Success:

```cpp
auto result = Result<void>::success();
```

Failure:

```cpp
auto result = Result<void>::failure(error);
```

### 9.5 Access Semantics

Calling `value()` when the result represents failure is a programming error and must be reported explicitly.

Calling `error()` when the result represents success is also a programming error and must be reported explicitly.

The implementation should use `std::logic_error` for these invalid access operations.

## 10. Exception Policy

DharaniNirmana uses Result types for expected operational failures.

Examples include:

- Invalid parameters.
- Invalid terrain dimensions.
- Numerical range violations.
- Invalid mesh topology.
- Unsupported operations.
- Resource allocation failures, where they can be handled at the relevant boundary.

Exceptions are not the normal public mechanism for reporting these conditions.

Exceptions may still be used for programming errors, invalid Result access, and failures that cannot reasonably be represented through the documented result contract.

The implementation must distinguish between expected operation failures and programming misuse.

## 11. Error Propagation

Modules should propagate failures without losing useful diagnostic information.

Conceptually:

```cpp
auto generated = generator.generate(parameters);

if (!generated) {
    return core::Result<terrain::HeightField>::failure(
        generated.error()
    );
}
```

When additional context is useful, an error may be enriched with information about the operation that failed.

Error propagation must not silently convert a failure into success.

## 12. Type and Header Dependencies

Core may depend on standard library facilities including:

- `<variant>`
- `<optional>`
- `<string>`
- `<string_view>`
- `<stdexcept>`
- `<cmath>`
- `<limits>`
- `<type_traits>`
- `<utility>`
- `<cstddef>`

Only the headers required by each public declaration should be included.

Core must not include headers belonging to other DharaniNirmana modules or Vishwakarma.

## 13. Testing Requirements

### 13.1 Mathematical Types

Test:

- Default construction.
- Component initialization.
- Arithmetic operations.
- Dot product.
- Cross product.
- Length calculation.
- Zero-length normalization behavior.
- Large finite components.
- Non-finite components.
- Overflow and underflow-sensitive calculations.

### 13.2 Result<T>

Test:

- Successful construction.
- Failure construction.
- State inspection.
- Boolean conversion.
- Value access.
- Error access.
- Invalid access behavior.
- Copyable values.
- Move-only values.
- Move construction.
- Move assignment.
- Const and rvalue access.

### 13.3 Result<void>

Test:

- Successful construction.
- Failure construction.
- State inspection.
- Boolean conversion.
- Valid value access.
- Valid error access.
- Invalid value access.
- Invalid error access.

### 13.4 Error

Test:

- Error construction.
- Diagnostic preservation.
- Error copying.
- Error moving.
- Error category inspection.

## 14. Future Extensions

Core may eventually include additional foundational mathematical types and utilities.

Potential additions include:

- Vec2f and Vec3f.
- Mat3d and Mat4d.
- Quaternion.
- Bounding volumes.
- Numerical utility functions.
- Additional error categories.

New abstractions must remain generic and must not introduce dependencies on higher-level engine modules.

## 15. Related Documents

- [API_DESIGN.md](API_DESIGN.md) — Public API conventions.
- [TERRAIN_DATA_API.md](TERRAIN_DATA_API.md) — Terrain data representation.
- [GENERATION_API.md](GENERATION_API.md) — Terrain generation.
- [PROCESSING_API.md](PROCESSING_API.md) — Terrain processing.
- [GEOMETRY_API.md](GEOMETRY_API.md) — Geometry representation.
- [DEPENDENCY_RULES.md](../architecture/DEPENDENCY_RULES.md) — Module dependency constraints.
