
# DharaniNirmana Module Boundaries

## 1. Purpose

This document defines the responsibilities, public interfaces, dependencies, and architectural restrictions of every DharaniNirmana module.

The primary objective is to prevent unrelated responsibilities from becoming coupled as the engine grows.

DharaniNirmana is divided into five core modules and one optional integration module.

### Core Modules

1. Core
2. Terrain
3. Generation
4. Processing
5. Geometry

### Optional Integration

6. Rendering

Rendering demonstrates consumption of DharaniNirmana geometry through Vishwakarma.

It is not part of the mandatory core engine.

## 2. Module Overview

| Module | Namespace | Primary responsibility |
|---|---|---|
| Core | dharaninirmana::core | Shared primitives and error handling |
| Terrain | dharaninirmana::terrain | Terrain domain data |
| Generation | dharaninirmana::generation | Procedural terrain creation |
| Processing | dharaninirmana::processing | Terrain transformations |
| Geometry | dharaninirmana::geometry | Mesh construction and validation |
| Rendering | dharaninirmana::rendering | Optional graphics integration |

## 3. Dependency Rules

The permitted dependencies are:

| Module | Allowed dependencies |
|---|---|
| Core | None |
| Terrain | Core |
| Generation | Core, Terrain |
| Processing | Core, Terrain |
| Geometry | Core, Terrain |
| Rendering | Geometry, Vishwakarma |

Rules:

1. Core must remain independent of all other DharaniNirmana modules.
2. Terrain may use Core primitives and error handling.
3. Generation may consume Terrain and Core APIs.
4. Processing may consume Terrain and Core APIs.
5. Geometry may consume Terrain and Core APIs.
6. Rendering may consume Geometry and Vishwakarma.
7. No core module may depend on Rendering.
8. No core module may include Vishwakarma headers.
9. Modules must not access another module's private implementation.
10. Dependencies must be declared explicitly in the build system.

Generation and Processing are independent modules.

Neither should depend on the other.

## 4. Core Module

Namespace:

```cpp
dharaninirmana::core
```

### 4.1 Responsibility

Core provides shared low-level functionality required by other modules.

It must not contain terrain-specific algorithms or rendering functionality.

### 4.2 Public Types

Initial public types:

```cpp
Result<T>
Error
ErrorCode
Vec2d
Vec3d
```

### 4.3 Responsibilities

- Result-based error reporting.
- Common error representation.
- Basic mathematical primitives.
- Small reusable utilities.
- Shared low-level validation helpers where appropriate.

### 4.4 Must Not

- Store terrain samples.
- Generate terrain.
- Process heightfields.
- Build terrain meshes.
- Depend on Vishwakarma.
- Depend on OpenGL or GLFW.

### 4.5 Public Headers

```text
include/dharaninirmana/core/
├── result.hpp
├── error.hpp
├── error_code.hpp
├── vec2d.hpp
└── vec3d.hpp
```

Exact file grouping may be adjusted during implementation without changing the module boundary.

## 5. Terrain Module

Namespace:

```cpp
dharaninirmana::terrain
```

### 5.1 Responsibility

Terrain owns the domain representations used to describe terrain.

The initial representation is a regular heightfield.

### 5.2 Public Types

```text
TerrainOrigin
HeightField
```

### 5.3 TerrainOrigin

TerrainOrigin represents the world-space horizontal location of the first heightfield sample.

```cpp
struct TerrainOrigin {
    double x;
    double z;
};
```

It belongs to the Terrain namespace because it has terrain-specific semantics.

It must not be replaced with a generic Vec2d.

### 5.4 HeightField Responsibilities

HeightField owns:

- Width.
- Height.
- Horizontal sample spacing.
- Terrain origin.
- Elevation samples.

The sample storage is row-major.

For grid coordinate `(x, z)`:

```text
index = z * width + x
```

World coordinates are:

```text
worldX = origin.x + x * spacingX
worldZ = origin.z + z * spacingZ
worldY = heights[index]
```

### 5.5 Validation Responsibilities

HeightField must validate:

- Width and height are at least two.
- Dimension multiplication does not overflow.
- Sample storage size matches dimensions.
- Spacing is positive and finite.
- Origin values are finite.
- Elevation samples are finite.
- The final spatial extent is finite.

### 5.6 Ownership

HeightField owns its sample storage.

Read-only access may be provided through:

```cpp
std::span<const double>
```

The caller must not retain a view beyond the lifetime of its HeightField.

### 5.7 Must Not

- Generate procedural terrain.
- Apply terrain filters.
- Construct meshes.
- Depend on graphics libraries.
- Own rendering resources.

### 5.8 Public Headers

```text
include/dharaninirmana/terrain/
├── terrain_origin.hpp
└── height_field.hpp
```

## 6. Generation Module

Namespace:

```cpp
dharaninirmana::generation
```

### 6.1 Responsibility

Generation creates terrain from procedural algorithms.

It converts generation requests into valid Terrain domain objects.

### 6.2 Initial Public Types

```text
GenerationParameters
ValueNoiseParameters
ValueNoiseGenerator
```

Additional generators may be introduced later.

### 6.3 Configuration Ownership

Generation uses two configuration levels.

**Common generation configuration**

```text
GenerationParameters
```

Contains:

- Width.
- Height.
- Sample spacing.
- TerrainOrigin.
- Random seed.

**Algorithm-specific configuration**

```text
ValueNoiseParameters
```

Contains parameters specific to value noise.

### 6.4 Generator Construction

Algorithm-specific configuration belongs to the generator instance.

Conceptual API:

```text
auto generator =
    ValueNoiseGenerator::create(valueNoiseParameters);
```

The factory validates configuration and returns:

```cpp
Result<ValueNoiseGenerator>
```

The generator owns its validated algorithm-specific configuration.

### 6.5 Generation Operation

Conceptual API:

```cpp
Result<HeightField> generate(
    const GenerationParameters& parameters
) const;
```

GenerationParameters describes the requested output terrain.

The generator's algorithm-specific configuration is not passed again to generate.

### 6.6 Responsibilities

- Validate generator configuration.
- Implement procedural algorithms.
- Use deterministic random behavior when seeded.
- Generate elevation samples.
- Construct valid HeightField objects.
- Report generation failures.

### 6.7 Must Not

- Own graphics devices.
- Create windows.
- Build rendering resources.
- Modify Vishwakarma.
- Perform geometry rendering.
- Depend on Processing.
- Hide mutable global random state.

### 6.8 Public Headers

```text
include/dharaninirmana/generation/
├── generation_parameters.hpp
├── value_noise_parameters.hpp
└── value_noise_generator.hpp
```

## 7. Processing Module

Namespace:

```cpp
dharaninirmana::processing
```

### 7.1 Responsibility

Processing transforms or analyzes existing terrain data.

It operates on terrain representations without taking responsibility for their creation.

### 7.2 Initial Scope

Potential initial operations:

- Smoothing.
- Height scaling.
- Height offset.
- Clamping.
- Normalization.
- Basic terrain analysis.

Individual operations should be introduced through focused APIs.

### 7.3 Input and Output

Processing operations must document whether they:

- Return a newly owned HeightField.
- Modify caller-owned data.
- Produce a separate analysis result.

The initial public design should favor operations that return new values when practical.

In-place operations may be introduced when justified by performance requirements.

### 7.4 Responsibilities

- Validate operation parameters.
- Apply terrain transformations.
- Preserve HeightField invariants.
- Report invalid operations.
- Define numerical behavior.

### 7.5 Must Not

- Generate procedural terrain.
- Own the HeightField domain representation.
- Build meshes.
- Depend on Generation.
- Depend on Geometry.
- Depend on Rendering or Vishwakarma.

### 7.6 Public Headers

```text
include/dharaninirmana/processing/
└── [operation-specific headers]
```

Exact headers will be added as processing capabilities are designed.

## 8. Geometry Module

Namespace:

```cpp
dharaninirmana::geometry
```

### 8.1 Responsibility

Geometry converts terrain representations into geometric structures suitable for downstream consumers.

It is independent of rendering APIs.

### 8.2 Initial Public Types

```text
Vertex
Mesh
MeshBuilder
```

### 8.3 Vertex

A Vertex contains:

- Position.
- Normal.

Positions and normals use DharaniNirmana mathematical types.

Initial precision is double.

### 8.4 Mesh

Mesh owns:

- Vertex storage.
- Triangle index storage.

The initial representation is an indexed triangle mesh.

Indices refer to entries in the vertex array.

### 8.5 MeshBuilder

MeshBuilder converts a HeightField into a Mesh.

It is responsible for:

- Vertex position construction.
- Triangle index generation.
- Normal calculation.
- Mesh validation.
- Reporting construction failures.

### 8.6 Triangle Winding

For grid samples:

```text
v00 = (x,     z)
v10 = (x + 1, z)
v01 = (x,     z + 1)
v11 = (x + 1, z + 1)
```

The initial triangle order is:

```text
Triangle 1: v00, v01, v10
Triangle 2: v10, v01, v11
```

This produces upward-facing normals for a flat terrain surface under the X/Z horizontal and Y-up convention.

### 8.7 Validation

Geometry must validate:

- Finite vertex positions.
- Finite normals.
- Valid index ranges.
- Nonzero triangle edges.
- Near-collinear triangle rejection.
- Valid normal generation.

The initial scale-aware angular degeneracy criterion is:

```text
sinTheta =
    length(cross(edge1, edge2)) /
    (length(edge1) * length(edge2))
```

Reject triangles when:

```text
sinTheta <=
    64 * std::numeric_limits<double>::epsilon()
```

The implementation must use numerically stable calculations to avoid overflow and underflow.

The threshold detects angular degeneracy, not a minimum absolute triangle area.

### 8.8 Must Not

- Generate terrain.
- Apply terrain processing algorithms.
- Depend on Vishwakarma.
- Create GPU buffers.
- Manage graphics resources.
- Render meshes.

### 8.9 Public Headers

```text
include/dharaninirmana/geometry/
├── vertex.hpp
├── mesh.hpp
└── mesh_builder.hpp
```

## 9. Rendering Module

Namespace:

```cpp
dharaninirmana::rendering
```

### 9.1 Responsibility

Rendering is an optional integration module demonstrating how DharaniNirmana geometry can be consumed through Vishwakarma.

It is not a core terrain engine responsibility.

### 9.2 Permitted Dependencies

Rendering may depend on:

- DharaniNirmana Geometry.
- DharaniNirmana Core types exposed through Geometry.
- Vishwakarma public graphics interfaces.

It may directly include:

```cpp
vishwakarma::graphics::IDevice
```

### 9.3 Responsibilities

- Consume DharaniNirmana Mesh objects.
- Convert indexed geometry into Vishwakarma's supported submission format.
- Submit triangle data.
- Report conversion and submission errors.
- Demonstrate a basic terrain visualization path.

### 9.4 Current Conversion Boundary

DharaniNirmana Mesh is indexed.

The current Vishwakarma drawing API accepts non-indexed triangle streams.

Rendering expands indexed triangles into interleaved seven-float vertex records:

```text
x, y, z, r, g, b, a
```

The source Mesh remains unchanged.

### 9.5 Ownership

Rendering borrows:

- The source Mesh.
- The graphics device.

The application owns:

- The window.
- The graphics device.
- The frame lifecycle.
- The event loop.
- Application shutdown.

### 9.6 Must Not

- Generate terrain.
- Process HeightField data.
- Modify Mesh objects.
- Own core terrain data.
- Become a dependency of any core module.
- Require graphics infrastructure when the integration is disabled.

### 9.7 Public Headers

```text
include/dharaninirmana/rendering/
├── render_parameters.hpp
└── mesh_renderer.hpp
```

## 10. Application Boundary

The application coordinates DharaniNirmana modules.

It decides:

- Which generator to configure.
- Which terrain to generate.
- Which processing operations to execute.
- When geometry should be constructed.
- Whether rendering is required.
- Which rendering integration to use.

The application owns orchestration.

DharaniNirmana modules should not introduce hidden application-level workflows.

## 11. Cross-Module Data Contracts

| Producer | Output | Consumer |
|---|---|---|
| Generation | HeightField | Application, Processing, Geometry |
| Processing | HeightField or analysis result | Application, Geometry |
| Geometry | Mesh | Application, Rendering, exporters |
| Rendering | Graphics submission | Vishwakarma |

The application controls the order of operations.

No module should require an unrelated module to perform its basic responsibility.

## 12. Error Handling Boundary

All modules use:

```cpp
dharaninirmana::core::Result<T>
```

Operations without a value use:

```cpp
dharaninirmana::core::Result<void>
```

Each module defines its own failure conditions while using the shared Error and ErrorCode abstractions.

Examples:

- Terrain reports invalid dimensions or sample storage.
- Generation reports invalid configuration or generation failure.
- Processing reports invalid operation parameters.
- Geometry reports invalid mesh data or degenerate triangles.
- Rendering reports invalid submission data or device errors.

Errors should be returned to the caller rather than silently ignored.

## 13. Build-System Boundary

The intended targets are:

```text
DharaniNirmana::Core
DharaniNirmana::Terrain
DharaniNirmana::Generation
DharaniNirmana::Processing
DharaniNirmana::Geometry
DharaniNirmana::Rendering
```

Rendering is optional.

The build option is:

```text
TERRAFORGE_BUILD_RENDERING
```

Default:

```text
OFF
```

When disabled, no core target may require Vishwakarma.

## 14. Testing Boundary

Each module owns its unit tests.

| Module | Testing focus |
|---|---|
| Core | Mathematical primitives and Result |
| Terrain | HeightField invariants and indexing |
| Generation | Configuration and deterministic generation |
| Processing | Transformation correctness |
| Geometry | Mesh topology, normals, and validation |
| Rendering | Conversion and graphics submission |

Core module tests must run without a window or graphics context.

Rendering conversion tests should be independently testable.

Graphics integration tests may require a graphics-capable environment.

## 15. Boundary Enforcement Rules

The following rules are mandatory:

1. Public headers expose only intentional API contracts.
2. Core modules must not include Vishwakarma headers.
3. Rendering is the only DharaniNirmana module permitted to include Vishwakarma headers.
4. No module may access another module's private implementation.
5. Domain data ownership must remain explicit.
6. Generator configuration must be validated during construction.
7. Processing behavior must document mutation semantics.
8. Geometry must remain renderer-independent.
9. Rendering must not modify source mesh data.
10. Applications own cross-module orchestration.

## 16. Completion Criteria

Module boundaries are established when:

- Every module has a defined responsibility.
- Public namespaces and headers are identified.
- Permitted dependencies are explicit.
- Ownership and lifetime are documented.
- Cross-module data contracts are clear.
- Rendering is isolated as an optional integration.
- Testing boundaries are independently enforceable.
