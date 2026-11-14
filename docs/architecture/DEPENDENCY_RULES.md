
# Dependency Rules

## 1. Purpose

This document defines the dependency rules for DharaniNirmana.

The primary objective is to maintain a graphics-independent terrain engine while allowing optional integration with external graphics libraries.

The architecture distinguishes two concerns:

1. DharaniNirmana Core Engine: terrain representation, generation, processing, and geometry construction.
2. Optional Rendering Integration: demonstration of consuming DharaniNirmana geometry through Vishwakarma.

The rendering integration is not a mandatory part of the core terrain engine.

## 2. Architectural Boundary

DharaniNirmana is divided into two logical parts.

### 2.1 Core Terrain Engine

The core engine provides:

- Core utilities.
- Terrain representation.
- Procedural generation.
- Terrain processing.
- Geometry construction.

These capabilities must work independently of graphics APIs.

The core engine must support headless execution.

### 2.2 Optional Rendering Integration

The Rendering integration demonstrates how applications can consume DharaniNirmana geometry using Vishwakarma.

It provides:

- Mesh conversion.
- Graphics device integration.
- Terrain mesh submission.
- Minimal rendering examples.

This integration is optional.

Applications that only generate, process, analyze, or export terrain must not require Vishwakarma.

## 3. Module Organization

The initial logical modules are:

| Module | Responsibility | Classification |
|---|---|---|
| Core | Foundational types and error handling | Core Engine |
| Terrain | HeightField representation | Core Engine |
| Generation | Procedural terrain generation | Core Engine |
| Processing | Terrain transformations | Core Engine |
| Geometry | Mesh representation and construction | Core Engine |
| Rendering | Vishwakarma integration | Optional Integration |
| Application | Coordinates application workflows | Application Layer |

The Rendering module is deliberately outside the core terrain engine.

## 4. Dependency Matrix

The following table defines permitted dependencies.

| Module | Permitted Dependencies |
|---|---|
| Core | None |
| Terrain | Core |
| Generation | Core, Terrain |
| Processing | Core, Terrain |
| Geometry | Core, Terrain |
| Rendering | Core, Geometry, Vishwakarma |
| Application | Required core modules and optional integrations |

A module must not depend on a higher-level module in a way that introduces a circular dependency.

## 5. Dependency Direction

The core engine follows a directed dependency structure.

```text
Core
  |
  v
Terrain
  |
  +------------------+
  |                  |
  v                  v
Generation       Processing
  |
  |
Geometry
  ^
  |
Rendering Integration
  |
  v
Vishwakarma
```

The diagram is conceptual. Generation and Processing are independent consumers of Terrain. Geometry also consumes Terrain directly.

Rendering consumes Geometry and integrates with Vishwakarma.

Rendering must never become a dependency of the core terrain modules.

## 6. Module Dependency Rules

### 6.1 Core

Core must not depend on:

- Terrain.
- Generation.
- Processing.
- Geometry.
- Rendering.
- Vishwakarma.
- OpenGL.
- GLFW.

Core provides foundational functionality only.

### 6.2 Terrain

Terrain may depend on Core.

Terrain must not depend on:

- Generation.
- Processing.
- Geometry.
- Rendering.
- Vishwakarma.
- Graphics APIs.

Terrain owns terrain data structures and their invariants.

### 6.3 Generation

Generation may depend on:

- Core.
- Terrain.

Generation must not depend on:

- Processing.
- Geometry.
- Rendering.
- Vishwakarma.
- Graphics APIs.

Generation produces Terrain Data representations.

### 6.4 Processing

Processing may depend on:

- Core.
- Terrain.

Processing must not depend on:

- Generation.
- Geometry.
- Rendering.
- Vishwakarma.
- Graphics APIs.

Processing transforms existing terrain representations.

### 6.5 Geometry

Geometry may depend on:

- Core.
- Terrain.

Geometry must not depend on:

- Generation.
- Processing.
- Rendering.
- Vishwakarma.
- Graphics APIs.

Geometry converts terrain representations into rendering-independent geometric data.

### 6.6 Rendering

Rendering may depend on:

- Core.
- Geometry.
- Vishwakarma.

Rendering must not introduce dependencies into the core engine.

It may directly consume:

```cpp
vishwakarma::graphics::IDevice
```

It may include Vishwakarma public headers and use its public APIs.

It must not access Vishwakarma private implementation details.

### 6.7 Application

Applications coordinate the core engine and optional integrations.

An application may depend on:

- Core.
- Terrain.
- Generation.
- Processing.
- Geometry.
- Rendering, when graphics integration is required.
- Vishwakarma, when the application owns its graphics lifecycle.

The application layer must not introduce dependencies from the core engine back into application-specific code.

## 7. CMake Target Organization

The initial CMake project should expose the following targets.

### Core Engine Targets

```text
DharaniNirmana::Core
DharaniNirmana::Terrain
DharaniNirmana::Generation
DharaniNirmana::Processing
DharaniNirmana::Geometry
```

These targets form the core terrain engine.

### Optional Integration Target

```text
DharaniNirmana::Rendering
```

This target is built only when rendering integration is enabled.

It links against:

```text
Vishwakarma::Graphics
```

### Target Dependency Requirements

| Target | Public Dependencies |
|---|---|
| DharaniNirmana::Core | None |
| DharaniNirmana::Terrain | DharaniNirmana::Core |
| DharaniNirmana::Generation | DharaniNirmana::Core, DharaniNirmana::Terrain |
| DharaniNirmana::Processing | DharaniNirmana::Core, DharaniNirmana::Terrain |
| DharaniNirmana::Geometry | DharaniNirmana::Core, DharaniNirmana::Terrain |
| DharaniNirmana::Rendering | DharaniNirmana::Core, DharaniNirmana::Geometry, Vishwakarma::Graphics |

Dependencies should use PRIVATE linkage unless consumers genuinely require the dependency's public types or headers.

Because the Rendering API directly exposes `vishwakarma::graphics::IDevice`, Vishwakarma must be available to consumers of that public interface. Its target dependency should therefore be PUBLIC unless the implementation later hides the device behind a DharaniNirmana-owned abstraction.

## 8. Optional Build Configuration

Rendering integration must be optional.

The initial configuration should support:

```cmake
option(
    DHARANINIRMANA_BUILD_RENDERING
    "Build optional Vishwakarma rendering integration"
    OFF
)
```

When disabled:

- Core engine targets remain available.
- No Vishwakarma dependency is required.
- No graphics context is required.
- Core tests can run in a headless environment.

When enabled:

- Vishwakarma must be available.
- The Rendering target is built.
- Rendering integration tests may be enabled.
- Example applications may demonstrate the full pipeline.

The exact Vishwakarma discovery mechanism will be defined in the CMake design.

## 9. Public Header Boundaries

Public headers must be organized by module.

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

Core engine headers must not include:

- GLFW headers.
- OpenGL headers.
- Vulkan headers.
- Vishwakarma graphics headers.

Rendering headers may include Vishwakarma public headers where required by their API contracts.

The Rendering target must not expose graphics dependencies through unrelated core targets.

## 10. Dependency Inversion

Core terrain algorithms must depend on domain abstractions rather than rendering implementations.

Examples:

- Generation produces HeightField.
- Processing consumes and produces HeightField.
- Geometry consumes HeightField and produces Mesh.
- Rendering consumes Mesh and submits it through Vishwakarma.

The core engine must not know how its output will be visualized.

A consumer may instead:

- Export the generated terrain.
- Perform analysis.
- Run simulation.
- Convert geometry into another representation.
- Submit geometry through a different renderer.

## 11. Prohibited Dependencies

The following dependencies are prohibited:

- Terrain depending on Generation.
- Generation depending on Processing.
- Processing depending on Geometry.
- Geometry depending on Rendering.
- Any core engine module depending on Vishwakarma.
- Any core engine module depending on OpenGL or GLFW.
- Any core engine module requiring a graphics context.
- Circular module dependencies.
- Global mutable state shared between modules.
- Application-specific code inside foundational modules.

## 12. Testing Boundaries

Core engine tests must run without graphics infrastructure.

| Module | Graphics Context Required |
|---|---|
| Core | No |
| Terrain | No |
| Generation | No |
| Processing | No |
| Geometry | No |
| Rendering | Potentially |
| Application Integration | Potentially |

Rendering conversion logic should be tested independently of actual graphics submission wherever practical.

Integration tests may require a graphics-capable environment.

## 13. Ownership and Lifetime Rules

Every module must document ownership of its public data structures.

Initial conventions:

- Core types follow value semantics.
- HeightField owns elevation storage.
- Mesh owns vertex and index storage.
- Generators own their algorithm configuration.
- Processing operations do not mutate their input.
- Rendering does not own source meshes.
- Rendering does not own the graphics device.
- Applications manage window and device lifecycles.

Non-owning references and views must have documented lifetime requirements.

## 14. Future Extension Policy

New modules may be introduced when they represent distinct responsibilities.

Potential future modules include:

- Serialization.
- Import and Export.
- Terrain Analysis.
- Spatial Partitioning.
- Terrain Streaming.
- Level of Detail.
- Simulation.
- GPU Processing.

A new module must define:

- Its responsibility.
- Its public API.
- Its permitted dependencies.
- Its ownership model.
- Its testing boundaries.

New functionality must not introduce unnecessary dependencies into the core terrain engine.

## 15. Completion Criteria

The dependency architecture is considered complete when:

- Core engine boundaries are explicit.
- Rendering is an optional integration.
- Vishwakarma is isolated from core engine modules.
- Dependency direction is documented.
- CMake targets are defined.
- Optional build behavior is specified.
- Public header boundaries are documented.
- Testing boundaries are explicit.
- Ownership and lifetime conventions are documented.
