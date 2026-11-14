
# DharaniNirmana API Design

## 1. Purpose

This document defines the public API conventions and design principles for DharaniNirmana.

It establishes consistent rules for:

- Public interfaces.
- Namespace organization.
- Header organization.
- Type and function naming.
- Error handling.
- Ownership and lifetime.
- Configuration.
- Validation.
- Module dependencies.
- API evolution.

Individual module API documents define the detailed contracts.

This document establishes conventions shared across those modules.

## 2. Language and Build Standards

Initial implementation requirements:

- C++20.
- CMake.
- Standard C++ library wherever practical.
- No mandatory graphics dependency for core engine modules.
- Independent unit testing for each core module.

The implementation must compile using the project's configured C++20 toolchain.

Compiler extensions should be avoided unless explicitly justified.

## 3. Namespace Organization

The root namespace is:

```text
dharaninirmana
```

Public modules use the following namespaces:

| Module | Namespace |
|---|---|
| Core | dharaninirmana::core |
| Terrain | dharaninirmana::terrain |
| Generation | dharaninirmana::generation |
| Processing | dharaninirmana::processing |
| Geometry | dharaninirmana::geometry |
| Rendering | dharaninirmana::rendering |

Rendering is an optional integration module.

Core modules must not include or depend on Rendering.

## 4. Public Header Organization

Public headers must be organized by module.

Initial organization:

```text
include/
└── dharaninirmana/
    ├── core/
    ├── terrain/
    ├── generation/
    ├── processing/
    ├── geometry/
    └── rendering/
```

Public headers must:

- Include their direct dependencies.
- Avoid relying on transitive includes.
- Avoid exposing implementation-only details.
- Use include guards or `#pragma once` consistently.
- Keep declarations focused and cohesive.
- Avoid unnecessary templates and complex metaprogramming.

Implementation details belong in source files unless templates or inline definitions are required.

Rendering headers may expose Vishwakarma public types.

Core headers must never require Vishwakarma headers.

## 5. Naming Conventions

### Types

Use PascalCase.

Examples:

```text
HeightField
TerrainOrigin
Mesh
MeshBuilder
ValueNoiseGenerator
RenderParameters
```

### Functions and Methods

Use camelCase.

Examples:

```text
create()
generate()
sample()
build()
process()
render()
```

### Variables

Use camelCase.

Examples:

```text
sampleSpacing
terrainOrigin
vertexCount
```

### Constants

Use a consistent project-wide convention.

Prefer descriptive names over abbreviations.

### Files

Use snake_case.

Examples:

```text
height_field.hpp
terrain_origin.hpp
mesh_builder.hpp
render_parameters.hpp
```

## 6. Public API Design Principles

### 6.1 Explicit Contracts

Public APIs must make the following clear:

- Required inputs.
- Valid input ranges.
- Ownership.
- Lifetime.
- Mutation behavior.
- Failure conditions.
- Output guarantees.

Avoid hidden behavior and implicit state changes.

### 6.2 Value Semantics

Prefer value types for domain data.

Examples:

- TerrainOrigin.
- Vec2d.
- Vec3d.
- GenerationParameters.
- RenderParameters.

Use references for borrowing existing objects.

Use smart pointers when ownership or polymorphic lifetime management requires them.

### 6.3 Const Correctness

Read-only operations should use const-qualified interfaces.

Examples:

```cpp
const HeightField&
const Mesh&
const std::vector<double>&
```

Use const methods when the operation does not modify observable object state.

### 6.4 Avoid Premature Abstraction

Do not introduce interfaces, factories, inheritance hierarchies, or generic frameworks without a concrete requirement.

Prefer small, composable components.

## 7. Error Handling Convention

### 7.1 Result Type

DharaniNirmana provides its own Result abstraction:

```cpp
dharaninirmana::core::Result<T>
```

The error type is incorporated into Result.

Do not use a separate `Result<T, Error>` public convention.

The conceptual representation is:

```cpp
Result<T> =
    std::variant<T, Error>
```

The actual implementation uses explicit success and failure factories.

### 7.2 Construction

Use:

```cpp
Result<T>::success(value)
Result<T>::failure(error)
```

Implicit construction from values or errors is not permitted.

### 7.3 Void Results

Operations that do not return a value use:

```cpp
Result<void>
```

The void specialization represents success without a stored error and failure with a stored Error.

### 7.4 Error Access

The Result API must clearly distinguish success and failure.

Expected operations include:

```text
hasValue()
value()
error()
```

Accessing the value of a failed result or the error of a successful result is invalid.

The initial implementation uses standard-library checked variant access behavior.

### 7.5 Error Categories

The initial error categories are:

- InvalidArgument.
- InvalidData.
- InvalidState.
- UnsupportedOperation.
- ResourceExhausted.
- InternalError.

Add new categories only when they represent a meaningful distinction.

### 7.6 Exceptions

Exceptions may still arise from standard-library operations, allocation, or programming errors.

The Result abstraction defines the expected failure-reporting convention for public engine operations.

Do not silently convert unexpected programming errors into arbitrary error categories.

## 8. Mathematical Types

Core mathematical types belong to:

```cpp
dharaninirmana::core
```

Initial types:

```text
Vec2d
Vec3d
```

Use double precision for terrain and geometry calculations unless a specific API boundary requires another representation.

Mathematical types should remain independent of graphics libraries.

They should provide only the operations needed by the engine.

## 9. Terrain Coordinate Convention

DharaniNirmana uses the following world-axis convention:

- X: horizontal axis.
- Z: horizontal axis.
- Y: elevation.

Terrain samples lie on the X-Z plane.

Height values represent Y coordinates.

### 9.1 TerrainOrigin

Terrain placement uses a dedicated type:

```cpp
struct TerrainOrigin {
    double x;
    double z;
};
```

Namespace:

```cpp
dharaninirmana::terrain
```

Header:

```text
terrain_origin.hpp
```

TerrainOrigin represents the world-space X and Z location of the first heightfield sample.

Do not use Vec2d to represent terrain origin.

This distinction prevents accidental mixing of generic mathematical vectors and terrain placement semantics.

### 9.2 Heightfield Mapping

For a regular heightfield:

```text
worldX = origin.x + x * spacingX
worldZ = origin.z + z * spacingZ
worldY = heights[z * width + x]
```

All modules must use the same coordinate convention.

## 10. Configuration Convention

Configuration types should be explicit and grouped according to responsibility.

### 10.1 Common Configuration

Parameters shared by terrain generation belong to:

```text
GenerationParameters
```

They include:

- Width.
- Height.
- Sample spacing.
- TerrainOrigin.
- Random seed.

### 10.2 Algorithm-Specific Configuration

Algorithm-specific parameters belong to dedicated configuration types.

For example:

```text
ValueNoiseParameters
```

A generator is configured when it is created.

Conceptual API:

```text
auto generator =
    ValueNoiseGenerator::create(parameters);

if (!generator) {
    // Handle configuration error.
}

auto terrain =
    generator.value().generate(generationParameters);
```

The generator owns its algorithm-specific configuration.

Generation-time parameters describe the requested output terrain.

This keeps generator configuration separate from individual generation requests.

### 10.3 Validation

Configuration must be validated before use.

Invalid configurations must not create partially initialized generator objects.

Use Result-based factory methods when construction requires validation.

## 11. Ownership and Lifetime

Ownership must be explicit.

### 11.1 Owned Data

Domain objects such as HeightField and Mesh own their underlying storage.

### 11.2 Borrowed Data

Read-only consumers should borrow through const references or const views.

Examples:

```cpp
const HeightField&
const Mesh&
std::span<const double>
```

Borrowed references and views must not outlive their source objects.

### 11.3 Transfer of Ownership

Use move semantics when ownership transfer is intended.

Do not introduce shared ownership by default.

Use `std::unique_ptr` for exclusive polymorphic ownership when appropriate.

## 12. Validation Convention

Validate public inputs at module boundaries.

Validation should cover:

- Dimensions.
- Storage sizes.
- Index ranges.
- Finite numeric values.
- Positive spacing.
- Coordinate validity.
- Configuration constraints.
- Geometric degeneracy.

Avoid repeating expensive validation in every internal helper when the input has already been validated and ownership guarantees remain intact.

Public operations must not assume arbitrary external data is valid.

## 13. Numerical Stability

Terrain generation and geometry calculations must use numerically sound operations.

Requirements:

- Reject non-finite input values.
- Avoid unchecked integer multiplication.
- Avoid unsafe floating-point conversions.
- Use scale-aware geometric checks.
- Avoid arbitrary absolute tolerances when a relative or dimensionless criterion is appropriate.
- Document numerical thresholds and their meaning.

Mesh triangle validation uses a dimensionless angular degeneracy criterion.

The initial threshold is:

```text
64 * std::numeric_limits<double>::epsilon()
```

This threshold detects nearly collinear triangle edges.

It is not an absolute minimum triangle-area requirement.

## 14. Module Dependency Convention

The core engine consists of:

- Core.
- Terrain.
- Generation.
- Processing.
- Geometry.

The Rendering module is optional.

Dependency rules:

- Core must not depend on other DharaniNirmana modules.
- Terrain may depend on Core.
- Generation may depend on Core and Terrain.
- Processing may depend on Core and Terrain.
- Geometry may depend on Core and Terrain.
- Rendering may depend on Geometry and Vishwakarma.
- No core module may depend on Rendering.
- No core module may depend on Vishwakarma.

Rendering is the only module permitted to consume Vishwakarma directly.

Because Rendering's public API may expose `vishwakarma::graphics::IDevice`, its Vishwakarma dependency must be available to consumers of that API.

## 15. CMake Target Convention

The intended public targets are:

```text
DharaniNirmana::Core
DharaniNirmana::Terrain
DharaniNirmana::Generation
DharaniNirmana::Processing
DharaniNirmana::Geometry
DharaniNirmana::Rendering
```

Rendering is optional and controlled by:

```text
TERRAFORGE_BUILD_RENDERING
```

The option should default to OFF.

When disabled:

- Core engine targets must still build.
- Core tests must still run.
- Vishwakarma must not be required.
- No graphics context must be created.

The exact target organization will be finalized in the build-system design.

## 16. API Evolution

Public API changes should be deliberate.

Before introducing a new public type or method, consider:

- Whether an existing abstraction is sufficient.
- Whether the new API creates unwanted dependencies.
- Whether ownership remains clear.
- Whether error handling is consistent.
- Whether the API is testable.
- Whether the API can evolve without breaking unrelated modules.

Avoid exposing implementation details that are likely to change.

## 17. Testing Convention

Every core module must have independent tests.

Tests should cover:

- Valid input.
- Invalid input.
- Boundary conditions.
- Error propagation.
- Ownership behavior.
- Deterministic results where applicable.
- Numerical edge cases.

Core tests must not require a graphics device or window.

Rendering conversion tests should be separable from graphics integration tests.

## 18. Documentation Convention

Every public module should document:

- Purpose.
- Public types.
- Public functions.
- Input constraints.
- Output guarantees.
- Ownership.
- Error conditions.
- Dependencies.
- Examples.

Detailed contracts belong in the corresponding module API document.

This document defines only conventions shared across modules.

## 19. Resolved Design Decisions

The following decisions are established for the initial implementation:

| Concern | Decision |
|---|---|
| Language | C++20 |
| Result abstraction | DharaniNirmana-owned Result<T> |
| Result storage | std::variant<T, Error> |
| Void result | std::optional<Error> specialization |
| Terrain placement | TerrainOrigin |
| Coordinate convention | X/Z horizontal, Y elevation |
| Generator configuration | Configure at construction |
| Algorithm configuration | Dedicated parameter types |
| Mesh representation | Indexed mesh |
| Mesh validation | Scale-aware angular degeneracy check |
| Rendering | Optional integration |
| Graphics foundation | Vishwakarma |
| Rendering dependency | Rendering module only |
| Rendering default | Disabled |

## 20. Completion Criteria

The API design conventions are established when:

- Namespace organization is consistent.
- Header organization is defined.
- Naming conventions are documented.
- Result semantics are consistent.
- Terrain coordinate semantics are explicit.
- Configuration ownership is clear.
- Ownership and lifetime rules are documented.
- Validation expectations are consistent.
- Numerical stability requirements are documented.
- Module dependency rules are explicit.
- Optional rendering is separated from the core engine.
- Testing conventions are established.
