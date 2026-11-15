
# DharaniNirmana Component Model

## 1. Purpose

This document describes the logical components inside DharaniNirmana and how they collaborate to provide terrain generation, processing, geometry construction, and optional rendering integration.

The component model complements the module boundaries.

- Module boundaries define dependency and ownership restrictions.
- The component model defines responsibilities and interactions inside those boundaries.

DharaniNirmana uses small, focused components with explicit contracts.

## 2. Component Overview

The initial component set is:

| Component | Module | Responsibility |
|---|---|---|
| Result and Error | Core | Failure reporting |
| Aryabhatta Mathematical Types | Aryabhatta | Mathematical types and operations used directly by engine APIs |
| TerrainOrigin | Terrain | Terrain placement |
| HeightField | Terrain | Terrain sample storage |
| Generation Configuration | Generation | Common generation request |
| Algorithm Configuration | Generation | Generator-specific settings |
| Terrain Generator | Generation | Procedural terrain creation |
| Terrain Processor | Processing | Terrain transformation |
| Mesh | Geometry | Indexed geometric storage |
| Mesh Builder | Geometry | HeightField-to-Mesh conversion |
| Mesh Validator | Geometry | Mesh invariant validation |
| Mesh Renderer | Rendering | Optional graphics submission |

Rendering components are optional and are not required by the core engine.

## 3. Component Relationships

```text
+--------------------------------------------------+
|                   Application                    |
|                                                  |
|            Coordinates component usage           |
+--------------------------+-----------------------+
                           |
          +----------------+----------------+
          |                |                |
          v                v                v
    Generation         Processing        Geometry
          |                |                |
          v                v                |
      HeightField <--------+                |
          |                                 |
          +---------------------------------+
                           |
                           v
                      Mesh Builder
                           |
                           v
                          Mesh
                           |
                           v
                 Optional Mesh Renderer
                           |
                           v
                      Vishwakarma
```

Shared engine infrastructure:

```text
Core
  |
  +---- Result and Error
```

Mathematical types are provided by Aryabhatta and used directly by engine APIs.

Terrain domain:

```text
TerrainOrigin
  |
  v
HeightField
```

The application controls orchestration.

Generation and Processing do not depend on one another.

## 4. Core Components

### 4.1 Result and Error

**Module:** Core

**Namespace:** dharaninirmana::core

Responsibilities:

- Represent successful operation results.
- Represent operation failures.
- Provide common error categories.
- Support value-returning and void operations.

Primary abstraction:

```cpp
Result<T>
```

The implementation uses:

```cpp
std::variant<T, Error>
```

Void operations use a dedicated Result<void> specialization based on:

```cpp
std::optional<Error>
```

Construction is explicit through success and failure factories.

All public modules use this shared error-reporting abstraction.

### 4.2 Mathematical Types

**Owner:** Aryabhatta

Engine APIs use `aryabhatta::math::DVec2` and `aryabhatta::math::DVec3` directly. Aryabhatta provides their mathematical operations; DharaniNirmana does not define aliases or duplicate vector implementations.

Use double precision for terrain and geometry calculations.

## 5. Terrain Components

### 5.1 TerrainOrigin

**Module:** Terrain

Purpose:

Represents the world-space horizontal origin of a terrain heightfield.

Initial representation:

```cpp
struct TerrainOrigin {
    double x;
    double z;
};
```

Responsibilities:

- Store world-space X and Z origin coordinates.
- Define the placement of the first heightfield sample.
- Preserve terrain-specific coordinate semantics.

It is not a general-purpose vector.

### 5.2 HeightField

**Module:** Terrain

Purpose:

Owns a regular grid of terrain elevation samples.

Responsibilities:

- Store dimensions.
- Store sample spacing.
- Store TerrainOrigin.
- Own elevation samples.
- Provide indexed read-only access.
- Preserve storage and coordinate invariants.

Initial storage:

```cpp
std::vector<double>
```

Indexing:

```text
index = z * width + x
```

Coordinate mapping:

```text
worldX = origin.x + x * spacingX
worldZ = origin.z + z * spacingZ
worldY = heights[index]
```

The component validates its invariants during construction.

HeightField owns its storage.

Read-only consumers may access sample data through const references or spans.

## 6. Generation Components

### 6.1 Generation Configuration

**Module:** Generation

Primary type:

```text
GenerationParameters
```

Purpose:

Describes common properties of the requested output terrain.

Initial fields:

- Width.
- Height.
- Sample spacing.
- TerrainOrigin.
- Random seed.

It does not contain algorithm-specific parameters.

### 6.2 Algorithm Configuration

**Module:** Generation

Initial type:

```text
ValueNoiseParameters
```

Purpose:

Contains parameters specific to the value noise algorithm.

The generator owns its algorithm-specific configuration.

Configuration is validated before the generator becomes usable.

### 6.3 Terrain Generator

**Module:** Generation

Initial implementation:

```text
ValueNoiseGenerator
```

Purpose:

Creates a HeightField using a configured procedural algorithm.

Construction:

```text
ValueNoiseGenerator::create(
    const ValueNoiseParameters&
)
```

Returns:

```cpp
Result<ValueNoiseGenerator>
```

Generation:

```text
generate(
    const GenerationParameters&
) const
```

Returns:

```cpp
Result<HeightField>
```

Responsibilities:

- Own validated algorithm configuration.
- Validate common generation parameters.
- Produce deterministic output for a fixed configuration and seed.
- Generate elevation samples.
- Construct a valid HeightField.
- Report failures through Result.

The generator does not own the resulting HeightField after returning it.

The caller receives ownership of the generated terrain.

### 6.4 Generation Interaction

```text
ValueNoiseParameters
         |
         v
ValueNoiseGenerator::create()
         |
         v
Configured Generator
         |
         | GenerationParameters
         v
generate()
         |
         v
     HeightField
```

Generator configuration and generation requests are separate concepts.

## 7. Processing Components

### 7.1 Terrain Processor

**Module:** Processing

Purpose:

Applies transformations or analysis operations to existing terrain data.

Potential initial operations:

- Smoothing.
- Height scaling.
- Height offset.
- Clamping.
- Normalization.
- Basic analysis.

Each operation should have a focused API.

### 7.2 Processing Contract

Processing operations must define:

- Input requirements.
- Parameter constraints.
- Mutation behavior.
- Output ownership.
- Numerical behavior.
- Failure conditions.

The initial design should favor producing a new HeightField where practical.

In-place operations may be added when performance requirements justify them.

### 7.3 Processing Interaction

```text
HeightField
     |
     v
Processing Operation
     |
     v
Result<HeightField>
```

Processing does not depend on Generation.

It operates on the Terrain domain representation.

## 8. Geometry Components

### 8.1 Vertex

**Module:** Geometry

Purpose:

Represents a geometric vertex.

Initial attributes:

- Position.
- Normal.

Both use `aryabhatta::math::DVec3` directly.

Initial numeric precision is double.

### 8.2 Mesh

**Module:** Geometry

Purpose:

Owns an indexed triangle mesh.

Initial data:

```cpp
std::vector<Vertex>
std::vector<std::uint32_t>
```

Responsibilities:

- Own vertex storage.
- Own index storage.
- Provide read-only access.
- Preserve mesh invariants.

Mesh is independent of rendering APIs.

### 8.3 Mesh Validator

**Module:** Geometry

Purpose:

Validates mesh structure and geometric properties.

Responsibilities:

- Check finite vertex positions.
- Check finite normals.
- Validate triangle indices.
- Reject zero-length edges.
- Reject nearly collinear triangles.
- Ensure normals are valid and normalizable.

Initial angular degeneracy criterion:

```text
sinTheta =
    length(cross(edge1, edge2)) /
    (length(edge1) * length(edge2))
```

Reject when:

```text
sinTheta <=
    64 * std::numeric_limits<double>::epsilon()
```

The implementation must use numerically stable calculations.

The threshold is dimensionless and detects angular degeneracy.

It does not define a minimum absolute triangle area.

### 8.4 Mesh Builder

**Module:** Geometry

Purpose:

Converts a HeightField into an indexed Mesh.

Responsibilities:

1. Validate input terrain data.
2. Generate vertex positions.
3. Generate triangle indices.
4. Calculate vertex normals.
5. Validate resulting geometry.
6. Return an owned Mesh.

### 8.5 Triangle Construction

For grid samples:

```text
v00 = (x,     z)
v10 = (x + 1, z)
v01 = (x,     z + 1)
v11 = (x + 1, z + 1)
```

Generate triangles:

```text
Triangle 1: v00, v01, v10
Triangle 2: v10, v01, v11
```

This winding produces upward-facing normals for flat terrain under the X/Z horizontal and Y-up convention.

### 8.6 Normal Generation

Normals are calculated using area-weighted triangle contributions.

For each triangle:

1. Calculate its unnormalized face normal using the cross product.
2. Add that contribution to each participating vertex.
3. Normalize accumulated vertex normals.
4. Reject invalid or non-normalizable results.

The builder and validator must use consistent geometric validity criteria.

### 8.7 Geometry Interaction

```text
HeightField
     |
     v
  MeshBuilder
     |
     +------> Mesh Validator
     |              |
     |              v
     +----------> Valid Mesh
```

The resulting Mesh owns its storage.

## 9. Optional Rendering Components

### 9.1 Mesh Renderer

**Module:** Rendering

**Namespace:** dharaninirmana::rendering

Purpose:

Demonstrates how a DharaniNirmana Mesh can be submitted to Vishwakarma.

The component is optional.

It must not be required by any core engine workflow.

### 9.2 Render Parameters

Initial configuration:

```text
RenderParameters
```

Contains a DharaniNirmana-owned color representation.

Color components must be finite and within [0, 1].

### 9.3 Mesh Conversion

Vishwakarma currently accepts non-indexed triangle streams.

DharaniNirmana Mesh is indexed.

The conversion component expands each indexed triangle into three interleaved vertex records.

The current record format is:

```text
x, y, z, r, g, b, a
```

Each record contains seven float values.

The conversion process:

1. Read triangle indices.
2. Resolve source vertices.
3. Convert positions from double to float.
4. Assign the configured color.
5. Append the expanded vertex records.
6. Submit the resulting stream.

The source Mesh remains unchanged.

Normals are retained in DharaniNirmana geometry but are not submitted through the current Vishwakarma interface.

### 9.4 Mesh Renderer Contract

Conceptual API:

```cpp
Result<void> render(
    vishwakarma::graphics::IDevice& device,
    const geometry::Mesh& mesh,
    const RenderParameters& parameters
) const;
```

Responsibilities:

- Validate rendering parameters.
- Validate source mesh data.
- Convert mesh data.
- Submit triangles.
- Propagate errors.

The renderer borrows the device and mesh.

It does not own either object.

### 9.5 Rendering Lifecycle

The application owns:

- Window creation.
- Device creation.
- Frame start.
- Frame end.
- Event processing.
- Shutdown.

MeshRenderer only submits mesh data during an active application-controlled frame.

It must not implicitly begin or end frames.

## 10. Component Interaction Scenarios

### 10.1 Generate Terrain

```text
Application
    |
    v
Configure ValueNoiseParameters
    |
    v
Create ValueNoiseGenerator
    |
    v
Supply GenerationParameters
    |
    v
Generate HeightField
    |
    v
Receive Result<HeightField>
```

### 10.2 Process Terrain

```text
Application
    |
    v
Existing HeightField
    |
    v
Processing Operation
    |
    v
Result<HeightField>
```

### 10.3 Build Geometry

```text
Application
    |
    v
HeightField
    |
    v
MeshBuilder
    |
    v
Mesh Validation
    |
    v
Result<Mesh>
```

### 10.4 Render Terrain

```text
Application
    |
    v
Create Window and Device
    |
    v
Begin Frame
    |
    v
MeshRenderer
    |
    v
Mesh Conversion
    |
    v
Vishwakarma Device Submission
    |
    v
End Frame
```

Rendering is optional.

The first three scenarios must work without a graphics context.

## 11. Ownership Model

| Component | Owns | Borrows |
|---|---|---|
| HeightField | Elevation samples | None |
| ValueNoiseGenerator | Algorithm configuration | Generation request |
| Processing operation | Its output, if newly allocated | Input terrain |
| Mesh | Vertices and indices | None |
| MeshBuilder | Temporary construction data | Input HeightField |
| MeshRenderer | Temporary conversion data | Mesh and device |
| Application | Window, device, orchestration | Engine objects as needed |

Ownership transfer should use move semantics when appropriate.

Borrowed references and views must not outlive their source objects.

## 12. Error Propagation

Every public component operation that can fail should use the shared Result abstraction.

Examples:

| Component | Possible failure |
|---|---|
| HeightField | Invalid dimensions or sample storage |
| Generator factory | Invalid algorithm configuration |
| Generate | Invalid request or resource exhaustion |
| Processing | Invalid operation parameters |
| MeshBuilder | Invalid terrain or geometry |
| Mesh Validator | Invalid indices or degenerate triangles |
| MeshRenderer | Invalid mesh or submission failure |

Errors should preserve useful context.

Do not silently ignore failures.

## 13. Execution Model

The initial engine is synchronous.

The application invokes operations explicitly.

No component should create hidden worker threads or background tasks.

This keeps execution deterministic and easier to test.

Future asynchronous processing may be introduced through explicit APIs.

Terrain generation and geometry construction must remain independent of graphics context requirements.

## 14. Testing Model

Each component must have focused unit tests.

### Core

- Result success and failure.
- Void Result behavior.
- Move-only values.
- Direct use of Aryabhatta types in public APIs.

### Terrain

- HeightField construction.
- Dimension validation.
- Storage validation.
- Indexing.
- Coordinate mapping.

### Generation

- Configuration validation.
- Generator construction.
- Deterministic output.
- Parameter validation.
- Generated HeightField invariants.

### Processing

- Transformation correctness.
- Boundary conditions.
- Mutation and ownership behavior.
- Numerical edge cases.

### Geometry

- Vertex construction.
- Triangle winding.
- Index generation.
- Normal calculation.
- Degenerate triangle rejection.
- Invalid mesh detection.

### Rendering

- Mesh expansion.
- Seven-float vertex layout.
- Float conversion.
- Color validation.
- Device submission.
- Error propagation.

Rendering conversion tests should be runnable without a graphics context.

## 15. Component Evolution

Future components may include:

- Additional procedural generators.
- Erosion processors.
- Terrain analysis.
- Mesh simplification.
- Mesh export.
- Terrain chunking.
- Level-of-detail generation.
- Alternative rendering adapters.

New components must preserve existing module boundaries.

Do not add dependencies between components merely for convenience.

## 16. Completion Criteria

The component model is established when:

- Every component has a focused responsibility.
- Component ownership is explicit.
- Public interactions are documented.
- Generator configuration and generation requests are separate.
- Processing is independent of Generation.
- Geometry remains renderer-independent.
- Mesh validation is consistent.
- Rendering is optional.
- Application orchestration remains outside engine components.
- Each component can be tested independently.
