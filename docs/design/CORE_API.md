# Core API

## 1. Purpose

The Core module provides foundational types and abstractions shared across DharaniNirmana.

It is the lowest-level module in the engine architecture and must remain independent of all other DharaniNirmana modules.

Core provides:

- Error representation.
- Explicit result types.
- Shared low-level utilities.

Core is responsible for exposing the foundational types required by the engine while avoiding unnecessary duplication of functionality already provided by established dependencies.

The Core API must be lightweight, predictable, and suitable for use throughout the engine.

## 2. Design Principles

The Core API follows these principles:

- **Minimal dependencies:** Depend only on the C++ standard library and explicitly approved foundational libraries.
- **Mathematical consistency:** Reuse Aryabhatta for mathematical types rather than maintaining duplicate vector implementations.
- **Explicit errors:** Operations that may fail communicate failure through Result types.
- **Value semantics:** Public mathematical types should behave as ordinary value types.
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
include/dharaninirmana/core/
├── result.hpp
├── error.hpp
└── error_code.hpp

src/core/
└── [private implementation files, if required]

tests/core/
└── [Core API tests]
```

Public headers define the Core API.

Core does not define or alias mathematical types. Aryabhatta owns mathematical types and operations, and consumers use its public types directly.

Exact implementation and test filenames may be determined as implementation is organized.

## 5. Mathematical Types

### 5.1 Mathematical Library

DharaniNirmana uses **Aryabhatta** as its foundational mathematical library.

Aryabhatta provides the mathematical types and operations required by the engine.

Engine modules use these types directly instead of implementing duplicate mathematical structures.

The dependency is exposed through the CMake target:

```cmake
Aryabhatta::Math
```

The Core target keeps the dependency public so consumers of public engine targets can resolve Aryabhatta types.

### 5.2 Direct Type Usage

Use Aryabhatta's double-precision types directly and include the public Aryabhatta header wherever they are used:

```cpp
#include <aryabhatta/math/math.hpp>

aryabhatta::math::DVec2 point;
aryabhatta::math::DVec3 position;
```

Aryabhatta owns these types and their operations. DharaniNirmana must not add another alias layer or independently implement vector storage or mathematical operations.

### 5.3 Mathematical Operations

Vector operations are provided by Aryabhatta and its underlying mathematical implementation.

Depending on the operation, consumers may use the corresponding Aryabhatta or GLM functionality.

Common operations include:

- Addition and subtraction.
- Scalar multiplication and division.
- Equality comparison.
- Dot product.
- Cross product for three-dimensional vectors.
- Length calculation.
- Normalization.

DharaniNirmana should not introduce wrapper functions for operations that are already adequately provided by Aryabhatta.

If the engine later requires domain-specific mathematical behavior, it should be introduced through an appropriately named engine-level abstraction rather than modifying the semantics of the underlying vector types.

### 5.4 Coordinate Convention

DharaniNirmana uses the following coordinate convention:

| Component | Meaning |
|---|---|
| X | Horizontal axis |
| Y | Elevation axis |
| Z | Horizontal axis |

The vector types themselves remain generic mathematical types.

They must not encode terrain-specific behavior.

Terrain-specific coordinate interpretation belongs to the Terrain module.

### 5.5 Numerical Behavior

Aryabhatta is responsible for the mathematical behavior of its vector types.

DharaniNirmana consumers must account for floating-point limitations when using these types.

In particular:

- Components may contain non-finite values unless an API explicitly requires finite vectors.
- Length calculations and other numerical operations must account for floating-point range limitations.
- Squared length calculations may overflow for large finite components.
- Normalization of zero-length vectors requires explicit consideration.
- Division by zero must not be treated as a valid numerical operation without a documented contract.

Core must not silently clamp or modify invalid mathematical inputs.

Where a DharaniNirmana operation requires finite vectors or other numerical constraints, the corresponding public API must document and validate those requirements.

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

An Error supports:

- An error category or code.
- A human-readable diagnostic message.
- Optional contextual information where useful.

Error categories should be stable enough for programmatic inspection.

Diagnostic messages may evolve as implementations improve.

### 6.4 Error Codes

The initial error categories are:

| Error Code | Meaning |
|---|---|
| `InvalidArgument` | An operation received an invalid argument. |
| `InvalidData` | Input data does not satisfy the required constraints. |
| `InvalidState` | An operation cannot be performed in the current state. |
| `UnsupportedOperation` | The requested operation is not supported. |
| `ResourceExhausted` | A required resource could not be obtained. |
| `InternalError` | An unexpected internal failure occurred. |

Additional categories may be introduced when justified by concrete API requirements.

### 6.5 Error Ownership

An Error owns its diagnostic information.

It must not retain references to temporary strings or caller-owned buffers.

Copying and moving an Error must preserve its diagnostic information.

## 7. Result Type

### 7.1 Purpose

`Result<T>` represents the outcome of an operation that may either succeed with a value or fail with an Error.

It avoids using exceptions as the normal mechanism for reporting expected operational failures.

The generic implementation is based on:

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

Results must be created through explicit factories.

Success:

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

The chosen access mechanism uses `std::logic_error`.

The implementation must not return an uninitialized or fabricated value.

Value access provides appropriate lvalue, const-lvalue, and rvalue overloads.

### 7.6 Error Access

Calling `error()` when the result contains a value is a programming error.

The implementation must report this misuse explicitly through `std::logic_error`.

The implementation must not fabricate an Error or return invalid storage.

Appropriate lvalue, const-lvalue, and rvalue overloads are provided.

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

The implementation uses `std::logic_error` for these invalid access operations.

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

### 12.1 Standard Library Dependencies

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

### 12.2 Aryabhatta Dependency

Core retains Aryabhatta as its approved foundational mathematical dependency. Engine APIs use Aryabhatta's mathematical types directly.

The public dependency is exposed through:

```cmake
target_link_libraries(
    DharaniNirmana_Core
    INTERFACE
    Aryabhatta::Math
)
```

This dependency remains public as Core's approved foundational mathematical dependency and propagates to consumers of engine targets that use Aryabhatta public types.

Consumers of DharaniNirmana Core must therefore be able to resolve the Aryabhatta dependency.

Aryabhatta's own package configuration is responsible for resolving its required dependencies.

### 12.3 Dependency Restrictions

Core must not include headers belonging to:

- DharaniNirmana Terrain.
- DharaniNirmana Generation.
- DharaniNirmana Processing.
- DharaniNirmana Geometry.
- DharaniNirmana Rendering.
- Vishwakarma.

Core must not depend on higher-level engine modules.

Aryabhatta is the approved foundational mathematical dependency.

## 13. Testing Requirements

### 13.1 Mathematical Type Usage

Test:

- Public headers that use mathematical vectors compile with explicit Aryabhatta includes.
- Geometry positions and normals use `aryabhatta::math::DVec3` directly.
- Geometry retains supplied double-precision values.

Tests should verify DharaniNirmana's public API usage rather than re-test Aryabhatta's mathematical implementation.

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

Core may eventually include additional foundational engine utilities. Mathematical types remain owned by Aryabhatta.

Potential additions include:

- Direct use of Aryabhatta matrices and quaternion types where required.
- Generic numerical utility functions.
- Additional error categories.
- Shared low-level utilities.

New abstractions must remain generic and must not introduce dependencies on higher-level engine modules.

Before adding a mathematical abstraction, evaluate whether Aryabhatta already provides the required functionality.

Avoid duplicating functionality that belongs in the mathematical library.

## 15. Related Documents

- [API_DESIGN.md](API_DESIGN.md) — Public API conventions.
- [TERRAIN_DATA_API.md](TERRAIN_DATA_API.md) — Terrain data representation.
- [GENERATION_API.md](GENERATION_API.md) — Terrain generation.
- [PROCESSING_API.md](PROCESSING_API.md) — Terrain processing.
- [GEOMETRY_API.md](GEOMETRY_API.md) — Geometry representation.
- [DEPENDENCY_RULES.md](../architecture/DEPENDENCY_RULES.md) — Module dependency constraints.