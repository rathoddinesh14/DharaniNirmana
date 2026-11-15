
# Geometry API

## 1. Purpose

The Geometry module converts terrain representations into geometric structures.

Its initial responsibility is to construct indexed triangle meshes from HeightField data.

It also provides mesh validation and vertex normal generation.

Geometry is a CPU-side representation layer. It must remain independent of graphics APIs and rendering infrastructure.

## 2. Design Principles

The Geometry API follows these principles:

- **Graphics independence:** Geometry must not depend on graphics devices or rendering libraries.
- **Explicit ownership:** Mesh objects own their vertex and index storage.
- **Indexed representation:** Shared vertices should be represented through indices rather than unnecessary duplication.
- **Consistent topology:** Mesh construction must use a documented triangle winding convention.
- **Numerical robustness:** Geometric calculations must account for floating-point overflow, underflow, and degeneracy.
- **Validation:** Invalid geometric data must be rejected explicitly.
- **Deterministic construction:** Identical valid inputs must produce identical mesh topology and vertex ordering.

## 3. Core Geometry Types

### 3.1 Vertex

A vertex represents a position and its associated normal.

Conceptual declaration:

```cpp
#include <aryabhatta/math/math.hpp>

namespace dharaninirmana::geometry {

struct Vertex {
    aryabhatta::math::DVec3 position;
    aryabhatta::math::DVec3 normal;
};

}
```

The position and normal use Aryabhatta's double-precision vector type directly. Aryabhatta owns the mathematical type and operations.

The position uses the engine's coordinate convention:

- X: Horizontal position.
- Y: Elevation.
- Z: Horizontal position.

The normal is a normalized direction vector when valid.

### 3.2 Mesh

A Mesh represents indexed triangle geometry.

Conceptual declaration:

```cpp
namespace dharaninirmana::geometry {

class Mesh {
public:
    std::span<const Vertex> vertices() const noexcept;
    std::span<const std::uint32_t> indices() const noexcept;

    std::size_t vertexCount() const noexcept;
    std::size_t indexCount() const noexcept;
    std::size_t triangleCount() const noexcept;

private:
    std::vector<Vertex> vertices_;
    std::vector<std::uint32_t> indices_;
};

}
```

The Mesh owns both vertex and index storage.

Public access should not permit uncontrolled mutation that bypasses validation.

The exact API may provide additional construction and inspection facilities, but ownership and validity guarantees must remain consistent.

## 4. Indexed Mesh Representation

A triangle mesh consists of:

- A vertex array.
- An index array.

Every group of three consecutive indices represents one triangle.

For a valid triangle mesh:

$$
indexCount \bmod 3 = 0
$$

Every index must refer to an existing vertex.

The initial mesh representation uses 32-bit unsigned indices.

The implementation must reject a mesh whose vertex count cannot be represented by the selected index type.

## 5. HeightField-to-Mesh Conversion

### 5.1 Purpose

The HeightField mesh builder converts a regular height field into an indexed triangle mesh.

Each height sample becomes one mesh vertex.

Adjacent samples are connected to form two triangles per grid cell.

### 5.2 Vertex Position

For sample coordinates $(i,j)$:

$$
x = origin_x + i\,spacingX
$$

$$
z = origin_z + j\,spacingZ
$$

$$
y = heights[j \cdot width + i]
$$

The resulting vertex position is:

$$
P_{i,j} = (x,y,z)
$$

### 5.3 Vertex Index

The vertex index is:

$$
V(i,j) = j \cdot width + i
$$

The builder must validate all size calculations and ensure that the resulting vertex indices are representable.

### 5.4 Cell Triangulation

For each grid cell, define:

- $v_{00}$: Top-left grid sample.
- $v_{10}$: Next sample along X.
- $v_{01}$: Next sample along Z.
- $v_{11}$: Diagonally opposite sample.

The cell is divided into two triangles:

```text
v00 -------- v10
 |          / |
 |        /   |
 |      /     |
 |    /       |
 |  /         |
v01 -------- v11
```

The triangle indices are:

```cpp
v00, v01, v10
v10, v01, v11
```

This winding convention produces upward-facing normals for a flat terrain under the engine's XZ-horizontal, Y-up coordinate system.

The builder must use the same winding convention consistently.

## 6. Triangle Count and Storage Validation

For a HeightField of width $W$ and depth $D$:

$$
vertexCount = W \cdot D
$$

$$
cellCount = (W-1)(D-1)
$$

$$
triangleCount = 2(W-1)(D-1)
$$

$$
indexCount = 6(W-1)(D-1)
$$

Every multiplication and addition used to calculate allocation sizes must be checked for integer overflow.

The builder must verify:

- HeightField dimensions are valid.
- Vertex count is representable.
- Index count is representable.
- All vertex positions are finite.
- All generated indices are valid.
- Required memory allocations can be completed.

The builder must not allocate based on unchecked size calculations.

## 7. Vertex Normal Generation

### 7.1 Purpose

Vertex normals provide a smooth directional representation of the mesh surface.

The initial implementation uses area-weighted accumulation of triangle normals.

### 7.2 Triangle Normal

For triangle vertices $A$, $B$, and $C$, define:

$$
e_1 = B-A
$$

$$
e_2 = C-A
$$

The unnormalized face normal is:

$$
N_f = e_1 \times e_2
$$

Its magnitude is twice the triangle area.

### 7.3 Area-Weighted Accumulation

For each triangle, accumulate its unnormalized face normal into the normals of its three vertices:

$$
N_A \mathrel{+}= N_f
$$

$$
N_B \mathrel{+}= N_f
$$

$$
N_C \mathrel{+}= N_f
$$

After processing all triangles, normalize each accumulated vertex normal.

This gives larger triangles proportionally greater influence.

### 7.4 Final Normalization

For an accumulated normal $N$:

$$
n = \frac{N}{\|N\|}
$$

A vertex with no valid contributing surface area cannot receive a meaningful normal.

The builder must reject such a mesh or report the invalid normal according to the documented construction policy.

The initial policy is to reject meshes containing vertices whose accumulated normal cannot be normalized.

## 8. Degenerate Triangle Detection

### 8.1 Definition

A degenerate triangle is a triangle whose geometry does not define a sufficiently stable surface normal.

Examples include:

- Repeated vertices.
- Zero-length edges.
- Collinear vertices.
- Nearly collinear vertices.

Such triangles must be rejected during mesh construction and mesh validation.

### 8.2 Angular Criterion

The degeneracy criterion is based on the angle between the two edges.

For non-zero edge vectors:

$$
\sin\theta =
\frac{\|e_1 \times e_2\|}
{\|e_1\|\|e_2\|}
$$

A triangle is considered degenerate when:

$$
\sin\theta \leq
64 \cdot \epsilon_{\text{double}}
$$

where:

$$
\epsilon_{\text{double}} =
\texttt{std::numeric\_limits<double>::epsilon()}
$$

This is a dimensionless angular criterion.

It does not impose an arbitrary minimum absolute triangle area.

### 8.3 Scale Independence

The criterion must be independent of uniform geometric scaling.

A geometrically similar triangle should not become degenerate merely because its coordinates are expressed at a different scale.

### 8.4 Consistency

The same degeneracy policy must be used by:

- HeightField mesh construction.
- General mesh validation.
- Triangle normal generation.

A triangle accepted by validation must satisfy the requirements needed by normal generation.

## 9. Numerical Robustness

Geometric calculations must account for the limitations of floating-point arithmetic.

Checking only the final cross product is insufficient.

Intermediate operations such as coordinate subtraction, multiplication, and accumulated normal addition may overflow or underflow.

### 9.1 Position Validation

Every vertex position must contain finite coordinates.

NaN and infinite positions are invalid.

### 9.2 Edge Construction

Edge vectors are computed from vertex positions.

The implementation must detect:

- Non-finite edge components.
- Zero-length edges.
- Unrepresentable edge differences.

A subtraction of two finite coordinates may still overflow.

Such cases must be rejected or handled through a numerically safer computation.

### 9.3 Stable Degeneracy Evaluation

The degeneracy test must not directly rely on an unscaled cross product and squared lengths when these operations may overflow or underflow.

A suitable strategy is:

1. Construct the two edge vectors using checked arithmetic.
2. Scale each edge independently by its maximum absolute component.
3. Reject zero-length scaled edges.
4. Compute the cross product of the scaled vectors.
5. Compute their scaled lengths using a stable norm calculation.
6. Evaluate the dimensionless angular criterion.
7. Reject non-finite intermediate or final values.

For scaled edge vectors $a$ and $b$:

$$
\sin\theta =
\frac{\|a \times b\|}
{\|a\|\|b\|}
$$

The result is independent of the original edge magnitudes.

The implementation must use a stable norm calculation, such as `std::hypot` or an equivalent scaled algorithm.

### 9.4 Face Normal Construction

The actual area-weighted face normal may be too large or too small to represent directly even when the triangle's orientation is valid.

The implementation must not assume that a valid angular test guarantees a representable unscaled cross product.

The implementation should use scaled arithmetic or another robust strategy to preserve the face-normal direction and relative area contribution.

If the required area-weighted quantity cannot be represented safely, the implementation must return an appropriate numerical failure rather than silently producing an invalid normal.

### 9.5 Normal Accumulation

Accumulating face normals may overflow even when individual face normals are finite.

The implementation must use a stable accumulation strategy.

Potential strategies include:

- Scaled accumulation.
- Compensated summation.
- Controlled normalization with preserved relative weights.
- A suitable combination of these techniques.

The selected implementation must preserve the intended area-weighted behavior within documented numerical limits.

### 9.6 Final Normal Validation

Every successful mesh must satisfy:

- All normal components are finite.
- Each normal has non-zero magnitude.
- Each normal is normalized within a documented floating-point tolerance.

The implementation must reject invalid normals rather than returning a mesh containing NaN or infinite values.

## 10. Mesh Validation

Mesh validation checks the structural and numerical validity of a Mesh.

### 10.1 Structural Validation

Validate:

- Index count is divisible by three.
- Every index refers to an existing vertex.
- Vertex storage is internally consistent.
- Index storage is internally consistent.

### 10.2 Position Validation

Every position must be finite.

### 10.3 Triangle Validation

For every triangle:

- All indices must be valid.
- All referenced positions must be finite.
- No edge may have zero length.
- The triangle must satisfy the angular degeneracy criterion.

### 10.4 Normal Validation

Every vertex normal must:

- Contain finite components.
- Have non-zero magnitude.
- Be normalized within the documented tolerance.

### 10.5 Validation Result

Mesh validation returns an explicit success or failure result.

The failure diagnostic should identify the relevant category and, where practical, the affected vertex or triangle index.

## 11. Ownership and Lifetime

A Mesh owns its vertex and index storage.

It must remain valid independently of the HeightField used to construct it.

The builder must not retain references to the source HeightField after construction.

Any spans returned by Mesh accessors are non-owning views.

Their lifetime is tied to the Mesh and may be invalidated by operations that modify its storage.

## 12. Error Handling

Public geometry operations return explicit results.

Representative signatures:

```cpp
core::Result<Mesh>
build(const terrain::HeightField& heightField);

core::Result<void>
validate(const Mesh& mesh);
```

Expected failure categories include:

| Category | Example |
|---|---|
| Invalid input | Invalid HeightField |
| Size overflow | Vertex or index count overflow |
| Index overflow | Vertex count exceeds index representation |
| Invalid position | Non-finite coordinate |
| Degenerate geometry | Zero-length edge |
| Angular degeneracy | Near-collinear triangle |
| Invalid normal | Zero or non-finite normal |
| Numerical failure | Unrepresentable intermediate or accumulation |
| Resource failure | Allocation failure |

A failed build must not expose a partially valid Mesh as a successful result.

## 13. Determinism

For identical valid HeightField inputs, the builder must produce identical:

- Vertex ordering.
- Vertex positions.
- Index ordering.
- Triangle winding.
- Normal results, subject to the documented floating-point reproducibility policy.

The implementation must not depend on unspecified iteration order or uninitialized storage.

## 14. Testing Requirements

### 14.1 Mesh Construction

Test:

- Minimum valid HeightField.
- Flat terrain.
- Sloped terrain.
- Non-uniform horizontal spacing.
- Non-zero terrain origin.
- Negative elevations.
- Large valid dimensions within practical test limits.

### 14.2 Topology

Verify:

- Correct vertex count.
- Correct triangle count.
- Correct index count.
- All indices are valid.
- Triangle winding is consistent.
- Flat terrain produces upward-facing normals.

### 14.3 Degeneracy

Test:

- Repeated vertices.
- Zero-length edges.
- Collinear triangles.
- Nearly collinear triangles.
- Valid triangles at different geometric scales.
- Very small and very large coordinate magnitudes.

### 14.4 Numerical Robustness

Test:

- Non-finite positions.
- Overflow during edge subtraction.
- Cross-product overflow.
- Cross-product underflow.
- Normal accumulation overflow.
- Invalid accumulated normals.
- Normalization of very small and very large vectors.

### 14.5 Ownership

Verify:

- Mesh remains valid after source HeightField destruction.
- Mesh owns its vertex storage.
- Mesh owns its index storage.
- Accessor views follow documented lifetime rules.

## 15. Future Extensions

The Geometry module may eventually support:

- Mesh simplification.
- Mesh subdivision.
- Mesh optimization.
- Tangent generation.
- Multiple surface representations.
- Adaptive terrain tessellation.
- Level-of-detail construction.
- Mesh analysis and geometric measurements.

These capabilities must preserve the module's graphics-independent design.

## 16. Related Documents

- `API_DESIGN.md` — Public API conventions.
- `CORE_API.md` — Mathematical types and error handling.
- `TERRAIN_DATA_API.md` — HeightField representation.
- `GENERATION_API.md` — Terrain generation.
- `PROCESSING_API.md` — Terrain processing.
- `RENDERING_API.md` — Optional rendering integration.
- `DEPENDENCY_RULES.md` — Module dependency constraints.
