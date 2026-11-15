
# DharaniNirmana Architecture Overview

## 1. Purpose

DharaniNirmana is a modular C++20 terrain generation and processing engine.

Its responsibility is to create, represent, transform, validate, and convert terrain data into geometry suitable for downstream applications.

The architecture prioritizes:

- Clear module boundaries.
- Independent core functionality.
- Explicit ownership and lifetime.
- Deterministic procedural generation.
- Composable processing operations.
- Numerically reliable geometry construction.
- Testability without graphics infrastructure.
- Optional integration with external graphics systems.

DharaniNirmana is not itself a rendering engine.

## 2. Architectural Scope

### 2.1 Core Engine

The core engine consists of five modules:

1. Core
2. Terrain
3. Generation
4. Processing
5. Geometry

These modules provide terrain functionality independently of any graphics API.

### 2.2 Optional Rendering Integration

Rendering is an optional integration module.

Its purpose is to demonstrate how DharaniNirmana geometry can be consumed by Vishwakarma.

It is not required for:

- Terrain generation.
- Terrain processing.
- Mesh construction.
- Geometry validation.
- Core engine testing.

Applications may use another renderer or export generated geometry directly.

## 3. High-Level Architecture

```text
+------------------------------------------------+
|                 Application                    |
|                                                |
|  Configures generation and processing         |
|  Controls application lifecycle               |
+------------------------+-----------------------+
                         |
                         v
+------------------------------------------------+
|               DharaniNirmana Core Engine           |
|                                                |
|  +-----------+                                 |
|  |   Core    |                                 |
|  +-----------+                                 |
|        ^                                       |
|        |                                       |
|  +-----------+                                 |
|  |  Terrain  |                                 |
|  +-----------+                                 |
|        ^                                       |
|        |                                       |
|  +------------+     +-------------+            |
|  | Generation |     | Processing  |            |
|  +------------+     +-------------+            |
|        |                   |                   |
|        +---------+---------+                   |
|                  |                             |
|                  v                             |
|             HeightField                        |
|                  |                             |
|                  v                             |
|             +----------+                       |
|             | Geometry |                       |
|             +----------+                       |
|                  |                             |
|                  v                             |
|                 Mesh                           |
+------------------+-----------------------------+
                   |
                   | Optional integration
                   v
+------------------------------------------------+
|             DharaniNirmana Rendering               |
|                                                |
|       Mesh conversion and submission           |
+------------------------+-----------------------+
                         |
                         v
+------------------------------------------------+
|                  Vishwakarma                   |
|                                                |
|          Graphics device and backend           |
+------------------------------------------------+
```

The diagram represents logical relationships rather than a strict sequence of execution.

Generation and Processing are independent operations that consume and produce terrain data.

Geometry consumes terrain data and produces mesh data.

Rendering consumes mesh data only when an application chooses to use the optional integration.

## 4. Module Responsibilities

### 4.1 Core

Namespace:

```cpp
dharaninirmana::core
```

Responsibilities:

- Result and error abstractions.
- Shared low-level utilities.
- Common validation helpers where appropriate.

Mathematical types and operations are owned by Aryabhatta. Engine APIs use Aryabhatta types directly.

Initial public types:

- Result<T>
- Error
- ErrorCode

Core must not depend on terrain-specific, geometry-specific, or graphics-specific functionality.

### 4.2 Terrain

Namespace:

```cpp
dharaninirmana::terrain
```

Responsibilities:

- Terrain domain representations.
- Heightfield storage.
- Terrain coordinate semantics.
- Terrain data validation.

Initial public types:

- TerrainOrigin
- HeightField

The initial terrain representation is a regular heightfield.

It owns its sample storage.

### 4.3 Generation

Namespace:

```cpp
dharaninirmana::generation
```

Responsibilities:

- Procedural terrain generation.
- Generator configuration.
- Deterministic random sampling.
- Conversion of generation requests into HeightField data.

Initial implementation:

- ValueNoiseGenerator
- ValueNoiseParameters
- GenerationParameters

Algorithm-specific parameters belong to the generator instance.

A generator is configured through a validated factory before generation requests are executed.

### 4.4 Processing

Namespace:

```cpp
dharaninirmana::processing
```

Responsibilities:

- Terrain transformations.
- Heightfield filtering.
- Terrain manipulation.
- Terrain analysis operations.

Processing operations consume terrain data and produce explicitly defined results.

An operation must document whether it modifies an input or returns a new object.

The initial design should favor clear value semantics over hidden mutation.

### 4.5 Geometry

Namespace:

```cpp
dharaninirmana::geometry
```

Responsibilities:

- Mesh construction.
- Vertex and index management.
- Normal generation.
- Geometric validation.
- Conversion from terrain representations into mesh data.

Initial public types:

- Vertex
- Mesh
- MeshBuilder

The initial mesh representation is indexed.

It is independent of rendering APIs.

### 4.6 Rendering

Namespace:

```cpp
dharaninirmana::rendering
```

Responsibilities:

- Optional integration with Vishwakarma.
- Mesh conversion into supported graphics submission formats.
- Submission through Vishwakarma's public device interface.
- Demonstration of terrain visualization.

Rendering must not generate or process terrain.

It must not become a dependency of the core engine.

## 5. Dependency Architecture

The intended dependency relationships are:

```text
Core
  ^
  |
Terrain
  ^
  |
+-------------------+
|                   |
Generation       Processing
|                   |
+---------+---------+
          |
          v
       Geometry
          |
          v
      Rendering
          |
          v
      Vishwakarma
```

This is a conceptual overview, not a statement that Generation and Processing depend on one another.

More precisely:

| Module | Permitted dependencies |
|---|---|
| Core | Aryabhatta |
| Terrain | Core |
| Generation | Core, Terrain |
| Processing | Core, Terrain |
| Geometry | Core, Terrain |
| Rendering | Geometry, Vishwakarma |

Rendering may also consume shared Core types through Geometry's public interfaces.

No core module may depend on Rendering or Vishwakarma.

## 6. Terrain Data Flow

The initial terrain pipeline uses HeightField as its central domain representation.

```text
GenerationParameters
        |
        v
Configured Generator
        |
        v
    HeightField
        |
        +-------------------+
        |                   |
        v                   v
   Processing          Geometry
        |                   |
        v                   v
    HeightField            Mesh
        |                   |
        +-------------------+
                            |
                            v
                   Optional Rendering
```

The application decides which operations to perform and in what order.

Generation, processing, and geometry construction should remain independently usable.

## 7. Terrain Coordinate Convention

DharaniNirmana uses:

- X: horizontal world axis.
- Z: horizontal world axis.
- Y: elevation.

The initial terrain representation is a regular X-Z heightfield.

Terrain placement uses:

```cpp
struct TerrainOrigin {
    double x;
    double z;
};
```

The origin identifies the world-space location of the first sample.

For a sample at grid coordinate `(x, z)`:

```text
worldX = origin.x + x * spacingX
worldZ = origin.z + z * spacingZ
worldY = heights[z * width + x]
```

All modules must follow this convention.

## 8. Data Ownership

The architecture uses explicit ownership.

### 8.1 Terrain

HeightField owns its height samples.

### 8.2 Geometry

Mesh owns its vertex and index storage.

### 8.3 Generation

Generators own their algorithm-specific configuration.

Generated HeightField objects own their output data.

### 8.4 Processing

Processing APIs document whether results are newly owned objects or modifications to caller-owned objects.

### 8.5 Rendering

Rendering borrows the source Mesh and graphics device.

It does not take ownership of either.

Temporary conversion buffers are owned by the rendering operation.

## 9. Error Handling

Public operations use:

```cpp
dharaninirmana::core::Result<T>
```

Operations without a return value use:

```cpp
dharaninirmana::core::Result<void>
```

Errors are represented using the DharaniNirmana Error abstraction.

Modules must report invalid input, invalid data, invalid state, unsupported operations, and resource exhaustion through the established error conventions.

The detailed Result contract is defined in [CORE_API.md](../design/CORE_API.md).

## 10. Rendering Integration Boundary

Vishwakarma provides the graphics foundation used by the optional integration.

The current integration uses:

```cpp
vishwakarma::graphics::IDevice
```

The current graphics submission API accepts non-indexed triangle vertex streams.

DharaniNirmana therefore expands indexed mesh triangles into the supported interleaved vertex format.

The current format is:

```text
x, y, z, r, g, b, a
```

The Rendering module performs this conversion.

The canonical DharaniNirmana Mesh remains indexed and unchanged.

The application owns:

- Window creation.
- Graphics device creation.
- Frame lifecycle.
- Event processing.
- Shutdown.

Rendering does not own the application lifecycle.

## 11. Build Architecture

The build system should expose independent core targets:

```text
DharaniNirmana::Core
DharaniNirmana::Terrain
DharaniNirmana::Generation
DharaniNirmana::Processing
DharaniNirmana::Geometry
```

The optional integration target is:

```text
DharaniNirmana::Rendering
```

Rendering is controlled through:

```text
DHARANINIRMANA_BUILD_RENDERING
```

The option defaults to OFF.

When disabled, the core engine must build and test without Vishwakarma or a graphics context.

## 12. Testing Architecture

### Core Tests

Test Error and Result behavior. Compile public API usage with explicit Aryabhatta headers where mathematical types are used.

### Terrain Tests

Test HeightField construction, storage, indexing, and validation.

### Generation Tests

Test configuration validation, deterministic output, and generated terrain properties.

### Processing Tests

Test individual transformations, numerical behavior, and input/output contracts.

### Geometry Tests

Test mesh construction, index validity, triangle degeneracy, and normal generation.

### Rendering Tests

Separate conversion tests from graphics integration tests.

Conversion tests should run without a graphics context.

Actual rendering tests may require a graphics-capable environment.

## 13. Architectural Constraints

The following constraints must remain true:

1. Core engine functionality is graphics-independent.
2. Terrain data owns its storage.
3. Generation configuration is validated at construction.
4. Processing behavior is explicit.
5. Geometry uses an indexed mesh representation.
6. Mesh validation uses numerically stable, scale-aware checks.
7. Rendering is optional.
8. Vishwakarma is consumed only by Rendering.
9. Applications control the rendering lifecycle.
10. Core tests do not require a graphics context.

## 14. Future Architectural Extensions

The architecture should permit future additions without requiring changes to existing core responsibilities.

Potential extensions:

- Additional terrain representations.
- Fractal and multi-octave generators.
- Hydraulic and thermal erosion.
- Terrain analysis algorithms.
- Mesh simplification.
- Terrain chunking.
- Level-of-detail generation.
- Export formats.
- Alternative rendering integrations.
- GPU-based processing.

These extensions must be introduced as separate capabilities with explicit dependency boundaries.

## 15. Completion Criteria

The architecture is considered established when:

- Core module responsibilities are clear.
- Dependency directions are explicit.
- Terrain coordinate semantics are consistent.
- Ownership boundaries are documented.
- Generation and processing remain independent.
- Geometry is graphics-independent.
- Rendering is optional and isolated.
- Build and testing boundaries are defined.
- Future extensions can be added without compromising the core architecture.
