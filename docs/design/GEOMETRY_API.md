
# Geometry API

## 1. Purpose

This document defines the public geometry representation and mesh construction API for DharaniNirmana.

The Geometry module converts terrain representations into general-purpose geometric data structures.

Its initial responsibility is to construct an indexed triangle mesh from a HeightField.

The resulting mesh must remain independent of rendering APIs.

## 2. Design Goals

The Geometry API must:

- Represent triangle meshes efficiently.
- Preserve vertex sharing through indexed topology.
- Support positions and normals.
- Generate consistent triangle winding.
- Produce valid mesh indices.
- Generate reliable vertex normals.
- Detect degenerate geometry.
- Remain independent of Vishwakarma.
- Be testable without a graphics context.

## 3. Namespace and Header Organization

Namespace:

```cpp
dharaninirmana::geometry
```

Initial public headers:

```text
include/dharaninirmana/geometry/
├── mesh.hpp
├── mesh_builder.hpp
└── mesh_validation.hpp
```

Implementation:

```text
src/geometry/
├── mesh.cpp
├── heightfield_mesh_builder.cpp
├── mesh_validation.cpp
└── normal_generator.cpp
```

Tests:

```text
tests/geometry/
├── mesh_tests.cpp
├── heightfield_mesh_builder_tests.cpp
└── normal_generator_tests.cpp
```

## 4. Mesh Data Model

### 4.1 Initial Representation

The initial representation is an indexed triangle mesh.

It contains:

- A collection of vertices.
- A collection of triangle indices.

Each vertex contains:

- Position.
- Normal.

### 4.2 Vertex Structure

Conceptual structure:

```cpp
struct Vertex {
    Vec3d position;
    Vec3d normal;
};
```

Both attributes use double precision.

Additional attributes such as UV coordinates, colors, and tangents are deferred.

### 4.3 Mesh Structure

Conceptual API:

```cpp
class Mesh {
public:
    std::span<const Vertex> vertices() const noexcept;

    std::span<const std::uint32_t> indices() const noexcept;

    std::size_t vertexCount() const noexcept;

    std::size_t indexCount() const noexcept;

    std::size_t triangleCount() const noexcept;
};
```

The actual implementation must define controlled construction and validation.

The Mesh owns its vertex and index storage.

External consumers receive read-only access.

## 5. Indexed Mesh Representation

An indexed mesh stores each logical vertex once and references it through indices.

Example:

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

Each group of three indices defines one triangle.

Advantages:

- Avoids duplicate vertex storage.
- Preserves shared vertices.
- Reduces CPU memory usage.
- Makes topology explicit.
- Allows efficient normal sharing.

The canonical DharaniNirmana mesh representation must remain indexed.

## 6. Triangle Topology

### 6.1 Triangle Indexing

The index array contains three indices per triangle.

Therefore:

```text
indexCount % 3 == 0
```

Triangle count:

```text
triangleCount = indexCount / 3
```

### 6.2 Index Validation

Every index must satisfy:

```text
index < vertexCount
```

Invalid indices must cause validation or construction to fail.

### 6.3 Winding Convention

DharaniNirmana uses:

- X: Horizontal axis.
- Y: Elevation.
- Z: Horizontal axis.

The initial terrain grid increases along positive X and positive Z.

For each cell, define:

```text
v00 = vertex(x,     z)
v10 = vertex(x + 1, z)
v01 = vertex(x,     z + 1)
v11 = vertex(x + 1, z + 1)
```

Generate:

```text
Triangle 1:
    v00, v01, v10

Triangle 2:
    v10, v01, v11
```

Under this coordinate convention, these triangles produce upward-facing geometric normals for a flat heightfield.

This winding convention must be preserved by the Mesh Builder.

## 7. HeightField to Mesh Conversion

### 7.1 Responsibility

The Mesh Builder converts a valid HeightField into an indexed triangle mesh.

Input:

```cpp
const HeightField& terrain
```

Output:

```cpp
Result<Mesh>
```

### 7.2 Vertex Generation

For each heightfield sample:

```text
worldX = origin.x + x * spacingX
worldY = height(x, z)
worldZ = origin.z + z * spacingZ
```

Create:

```text
Vertex {
    position = Vec3d(worldX, worldY, worldZ)
    normal = initially zero
}
```

The vertex count is:

```text
vertexCount = width * depth
```

Each heightfield sample corresponds to exactly one mesh vertex.

### 7.3 Index Generation

For each terrain cell:

```text
v00 = z * width + x
v10 = z * width + (x + 1)
v01 = (z + 1) * width + x
v11 = (z + 1) * width + (x + 1)
```

Generate:

```text
v00, v01, v10
v10, v01, v11
```

The number of quads is:

```text
quadCount = (width - 1) * (depth - 1)
```

The number of triangles is:

```text
triangleCount = 2 * quadCount
```

The number of indices is:

```text
indexCount = 6 * quadCount
```

All count calculations must be checked for integer overflow before allocation.

## 8. Normal Generation

### 8.1 Purpose

Normals describe the orientation of the mesh surface.

They are required for lighting calculations in rendering systems.

### 8.2 Face Normal

For a triangle with positions A, B, and C:

```text
edge1 = B - A
edge2 = C - A
```

The unnormalized face normal is:

```text
faceNormal = cross(edge1, edge2)
```

Its magnitude is proportional to twice the triangle area.

### 8.3 Vertex Normal

The initial implementation uses area-weighted vertex normals.

For every triangle:

1. Calculate the unnormalized face normal.
2. Add it to the accumulated normal of each triangle vertex.
3. After processing all triangles, normalize each accumulated vertex normal.

Conceptually:

```text
vertexNormal =
    normalize(sum(adjacentFaceNormals))
```

Area weighting avoids giving small and large triangles equal influence.

### 8.4 Normal Validity

Before normalization:

- The accumulated normal must be finite.
- Its magnitude must be greater than zero.
- It must not contain invalid numerical values.

If an accumulated normal is invalid or cannot be normalized, mesh construction must fail.

The implementation must not silently assign arbitrary normals.

### 8.5 Normal Orientation

Normals must be consistent with the triangle winding convention.

For a flat heightfield:

```text
normal approximately equals (0, 1, 0)
```

This provides a useful baseline test.

## 9. Mesh Degeneracy Policy

### 9.1 Purpose

A degenerate triangle has no reliable surface orientation.

Such triangles can cause:

- Undefined or unstable normals.
- Numerical instability.
- Invalid geometry.
- Rendering artifacts.

DharaniNirmana must reject degenerate triangles during mesh construction and mesh validation.

Both operations must use the same criterion.

### 9.2 Edge Validation

For a triangle with positions A, B, and C:

```text
edge1 = B - A
edge2 = C - A
```

Calculate the three edge lengths.

Every edge must have a finite, strictly positive length.

Triangles containing zero-length edges must be rejected.

Non-finite edge calculations must also cause rejection.

### 9.3 Scale-Aware Degeneracy Criterion

A fixed world-space area tolerance is unsuitable because terrain can have very different spatial scales.

Instead, use a dimensionless criterion based on the angle between the two edges.

Define:

```text
sinTheta =
    length(cross(edge1, edge2)) /
    (length(edge1) * length(edge2))
```

The triangle is rejected when:

```text
sinTheta <= tolerance
```

The initial tolerance is:

```text
tolerance =
    64 * std::numeric_limits<double>::epsilon()
```

This corresponds to a dimensionless threshold based on double-precision machine epsilon.

### 9.4 Interpretation

The criterion detects triangles whose two edges are:

- Exactly collinear.
- Numerically indistinguishable from collinear at the selected precision.

It does not reject triangles merely because their world-space area is small.

Consequently, geometrically similar triangles at different scales receive consistent treatment.

### 9.5 Numerical Implementation

The implementation must avoid unnecessary overflow and underflow when calculating the criterion.

Requirements:

- Check edge components for finiteness.
- Check edge lengths.
- Avoid directly multiplying extremely large edge lengths when calculating the denominator.
- Use scaled calculations where necessary.
- Ensure the final comparison is numerically meaningful.

The implementation may use normalized edge directions:

```text
u = normalize(edge1)
v = normalize(edge2)

sinTheta = length(cross(u, v))
```

Then apply:

```text
sinTheta <= tolerance
```

Normalization itself must be implemented safely for large and small finite vectors.

### 9.6 Scope of the Criterion

This criterion addresses angular degeneracy.

It does not impose:

- A minimum terrain size.
- A fixed minimum triangle area.
- A maximum terrain slope.
- A minimum world-space edge length.

Those are separate domain or application constraints.

### 9.7 Failure Behavior

A degenerate triangle causes:

```text
ErrorCode::InvalidData
```

The error should include sufficient context to identify the offending triangle where practical.

The Mesh Builder must not return a partially valid mesh after detecting a degenerate triangle.

## 10. Mesh Builder API

The initial public API should provide:

```cpp
class HeightFieldMeshBuilder {
public:
    Result<Mesh> build(
        const HeightField& terrain
    ) const;
};
```

The builder:

- Does not modify input terrain.
- Generates one vertex per heightfield sample.
- Generates two triangles per grid cell.
- Calculates vertex normals.
- Validates triangle geometry.
- Returns a validated Mesh.
- Does not create graphics resources.

A stateless implementation is preferred.

## 11. Mesh Validation

### 11.1 Responsibility

Mesh validation verifies structural and numerical correctness.

Conceptual API:

```cpp
Result<void> validateMesh(
    const Mesh& mesh
);
```

### 11.2 Structural Validation

Check:

- Index count is divisible by three.
- Every index references a valid vertex.
- Required vertex attributes exist.
- The mesh satisfies its documented topology requirements.

### 11.3 Numerical Validation

Check:

- Vertex positions are finite.
- Vertex normals are finite.
- Normals are normalized within the documented tolerance.
- Every triangle has valid edge lengths.
- Every triangle passes the scale-aware degeneracy criterion.
- No invalid geometry is silently accepted.

### 11.4 Validation Consistency

The same degeneracy helper and tolerance must be used by:

- HeightFieldMeshBuilder.
- General mesh validation.

Avoid maintaining separate degeneracy implementations that can produce inconsistent results.

### 11.5 Failure Behavior

Invalid meshes must produce explicit errors.

Validation must not modify the input mesh.

## 12. Mesh Ownership and Lifetime

Mesh exclusively owns:

- Vertex storage.
- Index storage.

The initial design follows value semantics.

Expected behavior:

- Copying a Mesh creates independent storage.
- Moving a Mesh transfers its storage.
- Read-only spans do not extend the lifetime of the Mesh.
- No graphics resources are owned by Mesh.

No shared ownership is required initially.

## 13. Error Handling

Mesh construction and validation use Core Result and Error types.

Expected failures include:

| Condition | Error Category |
|---|---|
| Invalid terrain input | InvalidData |
| Vertex count overflow | InvalidArgument |
| Index count overflow | InvalidArgument |
| Invalid triangle index | InvalidData |
| Non-finite vertex position | InvalidData |
| Invalid normal | InvalidData |
| Zero-length edge | InvalidData |
| Degenerate triangle | InvalidData |
| Allocation failure | ResourceExhausted |

Failed construction must not expose a partially initialized Mesh.

## 14. Performance Considerations

The initial implementation should:

- Allocate vertex and index storage once where practical.
- Traverse terrain samples in row-major order.
- Reserve safely calculated storage sizes.
- Avoid unnecessary vertex duplication.
- Avoid repeated allocation during normal generation.
- Use deterministic iteration order.

The degeneracy check should avoid unnecessary expensive operations while preserving numerical correctness.

Optimization must follow profiling.

Parallel mesh construction is deferred.

## 15. Testing Requirements

### Vertex Generation

- Correct vertex count.
- Correct world-space positions.
- Non-zero TerrainOrigin.
- Different X and Z spacing.
- Correct elevation mapping.

### Index Generation

- Correct triangle count.
- Correct index count.
- Correct index ranges.
- Correct triangle winding.
- Correct topology for minimum valid dimensions.

### Normal Generation

- Flat terrain produces upward normals.
- Sloped terrain produces consistent normals.
- Area-weighted accumulation is correct.
- Zero accumulated normal is rejected.
- Non-finite normals are rejected.

### Degeneracy Validation

- Normal triangle passes.
- Exactly collinear triangle fails.
- Nearly collinear triangle below tolerance fails.
- Triangle above tolerance passes.
- Zero-length edge fails.
- Very small but valid triangle passes.
- Very large but valid triangle passes.
- Scale-equivalent triangles produce consistent results.
- Non-finite coordinates fail.

### Mesh Validation

- Valid mesh passes.
- Index count not divisible by three fails.
- Out-of-range indices fail.
- Non-finite positions fail.
- Invalid normals fail.
- Degenerate triangles fail.

### Ownership

- Copy independence.
- Move behavior.
- Read-only access.
- No dependency on graphics resources.

All Geometry tests must run without a graphics context.

## 16. Initial Implementation Files

The initial Geometry module should contain:

```text
include/dharaninirmana/geometry/
├── mesh.hpp
├── mesh_builder.hpp
└── mesh_validation.hpp

src/geometry/
├── mesh.cpp
├── heightfield_mesh_builder.cpp
├── mesh_validation.cpp
└── normal_generator.cpp

tests/geometry/
├── mesh_tests.cpp
├── heightfield_mesh_builder_tests.cpp
└── normal_generator_tests.cpp
```

Files may be combined if separate compilation units add unnecessary complexity.

## 17. Future Extensions

Potential future geometry capabilities:

- UV coordinates.
- Tangent generation.
- Vertex colors.
- Multiple mesh attributes.
- Configurable triangulation.
- Mesh partitioning.
- Mesh simplification.
- Level-of-detail meshes.
- Mesh serialization.
- Alternative geometry representations.

These are outside the initial implementation scope.

## 18. Design Decisions Deferred

The following details will be finalized during implementation:

- Exact Mesh construction mechanism.
- Public versus private validation entry points.
- Error diagnostic format.
- Mesh index type evolution.
- Optional vertex attributes.
- Normal generation utility boundaries.
- Mesh bounds representation.

The degeneracy criterion and its tolerance are architectural decisions and must remain consistent across construction and validation.

## 19. Completion Criteria

The Geometry API is ready for implementation when:

- Vertex representation is defined.
- Indexed topology is specified.
- Triangle winding is explicit.
- HeightField conversion is documented.
- Normal generation is specified.
- Degeneracy criterion is mathematically defined.
- Numerical validation is specified.
- Ownership is explicit.
- Independent tests are identified.
