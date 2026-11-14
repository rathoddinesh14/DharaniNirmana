
# Rendering Integration API

## 1. Purpose

This document defines the optional rendering integration between DharaniNirmana and Vishwakarma.

The Rendering module demonstrates how applications can consume DharaniNirmana terrain and geometry APIs through an optional integration with Vishwakarma.

It is not a mandatory part of the core terrain engine.

DharaniNirmana's primary functionality is terrain representation, generation, processing, and geometry construction.

Rendering exists to demonstrate one possible visualization path.

Applications must be able to use the core engine without installing or linking Vishwakarma.

## 2. Design Goals

The Rendering integration must:

- Demonstrate consumption of DharaniNirmana geometry.
- Integrate with the existing Vishwakarma public API.
- Keep graphics dependencies outside the core engine.
- Submit DharaniNirmana meshes through the supported device interface.
- Preserve the canonical indexed mesh representation.
- Avoid modifying source mesh data.
- Respect graphics resource ownership and thread constraints.
- Support headless core engine builds.
- Remain replaceable by other rendering integrations.

## 3. Namespace and Header Organization

Namespace:

```cpp
dharaninirmana::rendering
```

Initial public headers:

```text
include/dharaninirmana/rendering/
├── mesh_renderer.hpp
└── render_parameters.hpp
```

Implementation:

```text
src/rendering/
├── mesh_renderer.cpp
└── mesh_conversion.cpp
```

Tests:

```text
tests/rendering/
├── mesh_renderer_tests.cpp
└── mesh_conversion_tests.cpp
```

The Rendering module is optional.

Only this module may depend on Vishwakarma.

Unlike core engine modules, Rendering headers may expose Vishwakarma public types when useful.

## 4. Architectural Position

The rendering integration consumes output from the core engine.

```text
Generation
     |
     v
HeightField
     |
     v
Processing
     |
     v
HeightField
     |
     v
Geometry
     |
     v
DharaniNirmana Mesh
     |
     v
Optional Rendering Integration
     |
     v
Vishwakarma
     |
     v
OpenGL Output
```

The core engine must not depend on Rendering or Vishwakarma.

Applications may choose another renderer or export geometry without using this integration.

## 5. Vishwakarma Integration Contract

DharaniNirmana integrates through:

```cpp
vishwakarma::graphics::IDevice
```

The current public interface provides:

```text
backend()
resize(width, height)
beginFrame(clearColor)
drawPoints(vertices, vertexCount)
drawLines(vertices, vertexCount)
drawTriangles(vertices, vertexCount)
endFrame()
name()
```

Device creation is performed through:

```cpp
vishwakarma::graphics::createDevice(
    Backend,
    platform::Window&
)
```

The currently supported graphics backend is OpenGL.

DharaniNirmana must use only the supported public API.

It must not access Vishwakarma's private OpenGL implementation.

## 6. Current Vertex Submission Format

Vishwakarma's triangle submission interface accepts an interleaved vertex stream.

Each vertex contains seven floating-point values:

```text
x, y, z, r, g, b, a
```

Therefore:

```text
floatsPerVertex = 7
```

The vertex count passed to drawTriangles is the number of vertices, not the number of floats.

The submission stream must contain exactly:

```text
vertexCount * 7
```

floating-point values.

The current interface does not accept normals, texture coordinates, or indexed topology.

## 7. Mesh Conversion Strategy

### 7.1 Canonical Representation

DharaniNirmana retains its indexed mesh representation:

- Indexed vertices.
- Triangle indices.
- Double-precision positions.
- Double-precision normals.

This representation is independent of Vishwakarma.

### 7.2 Conversion Requirement

Vishwakarma currently does not expose indexed mesh submission.

The Rendering Adapter must expand indexed triangles into a non-indexed vertex stream.

For each triangle:

1. Read its three indices.
2. Resolve the corresponding DharaniNirmana vertices.
3. Convert positions to float.
4. Assign the configured color.
5. Append the seven-float vertex records.

The expanded stream is temporary submission data.

The source mesh remains unchanged.

### 7.3 Conversion Example

Given:

```text
Vertices:
    V0
    V1
    V2
    V3

Indices:
    0, 1, 2
    2, 1, 3
```

The expanded stream contains:

```text
V0, V1, V2, V2, V1, V3
```

The output contains six vertex records.

### 7.4 Attribute Conversion

DharaniNirmana positions use double precision.

Vishwakarma submission uses float precision.

The adapter converts:

```cpp
double -> float
```

The conversion must reject values that cannot be represented as finite float values.

The initial adapter uses a uniform color supplied through rendering parameters.

Normals remain available in the DharaniNirmana mesh but are not submitted through the current seven-float interface.

Lighting based on vertex normals is deferred until the graphics API supports the required attributes or shader configuration.

## 8. Render Parameters

The initial rendering configuration contains a uniform color.

A DharaniNirmana-owned public type is preferred:

```cpp
struct RenderColor {
    float r;
    float g;
    float b;
    float a;
};
```

Conceptual configuration:

```cpp
struct RenderParameters {
    RenderColor color;
};
```

This prevents Vishwakarma types from becoming necessary in the rendering configuration itself.

The adapter converts RenderColor into the Vishwakarma color representation internally.

Requirements:

- All color components must be finite.
- Components must lie within the range [0, 1].
- The default color must be explicitly defined.
- Color validation must happen before submission.

The initial implementation should use a simple neutral terrain color.

Material systems and textures are deferred.

## 9. Mesh Renderer

### 9.1 Responsibility

MeshRenderer submits a DharaniNirmana Mesh to a supplied Vishwakarma device.

It does not own:

- The device.
- The source mesh.
- The application window.
- The application frame lifecycle.

### 9.2 Public API

The optional Rendering module may directly expose Vishwakarma types.

Conceptual API:

```cpp
class MeshRenderer {
public:
    Result<void> render(
        vishwakarma::graphics::IDevice& device,
        const geometry::Mesh& mesh,
        const RenderParameters& parameters
    ) const;
};
```

This dependency is acceptable because Rendering is explicitly an optional integration module.

It does not weaken the graphics-independent boundary of the core engine.

### 9.3 Responsibilities

MeshRenderer must:

- Validate the input mesh.
- Validate rendering parameters.
- Convert mesh data into the supported vertex stream.
- Submit triangles through the device.
- Return explicit errors when conversion or submission cannot proceed.

It must not:

- Generate terrain.
- Modify mesh geometry.
- Change mesh topology.
- Own the graphics device.
- Create windows.
- Manage the application event loop.

## 10. Rendering Lifecycle

The application owns the window and graphics device lifecycle.

The application is responsible for:

1. Creating a window.
2. Creating a graphics device.
3. Beginning a frame.
4. Submitting terrain meshes.
5. Ending a frame.
6. Managing events and shutdown.

Conceptual sequence:

```text
Create Window
     |
     v
Create Device
     |
     v
Begin Frame
     |
     v
MeshRenderer::render()
     |
     v
End Frame
     |
     v
Continue Application Loop
```

MeshRenderer must not implicitly begin or end frames.

Frame management remains an application responsibility.

## 11. Graphics Context and Threading

Vishwakarma requires graphics operations to respect its window and device lifecycle constraints.

The Rendering Adapter must:

- Submit commands only when the device is valid.
- Execute graphics operations on the appropriate context-owning thread.
- Avoid creating graphics resources from arbitrary worker threads.
- Avoid hidden thread creation.
- Avoid retaining device references beyond their documented lifetime.

Terrain generation and mesh construction may execute independently of the graphics context.

The initial Rendering API is synchronous.

## 12. Memory and Ownership

MeshRenderer does not own the input mesh or graphics device.

The caller must ensure both remain alive during rendering.

The expanded vertex stream is temporary data owned by the rendering operation.

The initial implementation may allocate a temporary vector for conversion.

Persistent GPU resource caching is deferred because the current Vishwakarma API does not expose the required buffer and resource management interfaces.

## 13. Error Handling

Rendering operations return:

```cpp
Result<void>
```

Expected failures include:

| Condition | Error Category |
|---|---|
| Invalid mesh | InvalidData |
| Invalid color | InvalidArgument |
| Invalid vertex position | InvalidData |
| Float conversion produces non-finite value | InvalidData |
| Invalid index | InvalidData |
| Unsupported operation | UnsupportedOperation |
| Temporary allocation failure | ResourceExhausted |
| Invalid rendering state | InvalidState |

The exact behavior for device-level failures must follow Vishwakarma's documented contract.

The adapter must not silently discard failed submissions.

## 14. Performance Considerations

The initial implementation prioritizes correctness.

Requirements:

- Validate mesh indices before expansion.
- Reserve temporary stream capacity when practical.
- Convert each referenced vertex as required by triangle submission.
- Avoid modifying canonical mesh storage.
- Avoid unnecessary intermediate copies.
- Do not introduce persistent GPU caching without a supported resource API.

The current non-indexed submission interface can increase temporary CPU memory usage.

This limitation is accepted for the initial integration.

Future indexed drawing support in Vishwakarma can eliminate the expansion step.

## 15. Testing Requirements

### Conversion Tests

- Correct vertex expansion.
- Correct triangle ordering.
- Correct seven-float layout.
- Correct position conversion.
- Correct color assignment.
- Correct output vertex count.
- Invalid index detection.
- Non-finite conversion detection.
- Source mesh remains unchanged.

### Renderer Tests

- Valid mesh submission.
- Empty or invalid mesh behavior.
- Invalid color rejection.
- Device submission invocation.
- Error propagation.

Where practical, isolate mesh conversion from actual graphics submission.

### Integration Tests

A small integration test should verify:

- Window creation.
- Device creation.
- Frame lifecycle.
- Terrain mesh submission.
- Clean shutdown.

Integration tests may require a graphics-capable environment.

Core engine tests must never require a graphics context.

## 16. Initial Implementation Files

The optional Rendering module should contain:

```text
include/dharaninirmana/rendering/
├── mesh_renderer.hpp
└── render_parameters.hpp

src/rendering/
├── mesh_renderer.cpp
└── mesh_conversion.cpp

tests/rendering/
├── mesh_conversion_tests.cpp
└── mesh_renderer_tests.cpp
```

A small example application should demonstrate the complete pipeline.

## 17. Current Limitations

The current Vishwakarma API does not provide:

- Indexed mesh drawing.
- General-purpose GPU buffers.
- Custom shader management.
- Texture management.
- Material management.
- Render graph functionality.
- Persistent mesh resource management.

DharaniNirmana must not assume these capabilities exist.

The initial integration must work with the actual public API.

These capabilities may be added to Vishwakarma independently in the future.

## 18. Future Extensions

Potential future rendering capabilities include:

- Indexed mesh submission.
- Vertex normal submission.
- Lighting and shader support.
- Material representation.
- Texture coordinates.
- Texture management.
- GPU mesh caching.
- Terrain chunk rendering.
- Level-of-detail rendering.
- Frustum culling.

These are outside the initial integration scope.

## 19. Design Decisions Deferred

The following details will be finalized during implementation:

- Exact conversion helper API.
- Device error translation.
- Temporary buffer reuse.
- Rendering test strategy.
- Example application structure.

The optional integration must preserve the graphics-independent core engine boundary.

## 20. Completion Criteria

The Rendering integration API is ready for implementation when:

- Its optional status is explicit.
- Vishwakarma integration is documented.
- Vertex submission format is documented.
- Indexed mesh expansion is defined.
- Rendering lifecycle ownership is clear.
- Graphics thread constraints are documented.
- Memory ownership is explicit.
- Error handling is defined.
- Testing requirements are identified.
- Missing Vishwakarma capabilities are acknowledged.
