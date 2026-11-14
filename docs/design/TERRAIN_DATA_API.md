
# Terrain Data API

## 1. Purpose

This document defines the public API and data model for DharaniNirmana's initial terrain representation.

The initial implementation uses a regular heightfield.

A heightfield represents terrain as a two-dimensional grid of elevation samples distributed over the X-Z plane.

The Terrain module owns this representation and provides controlled access to its data.

## 2. Design Goals

The HeightField API must:

- Represent regular terrain grids efficiently.
- Use contiguous memory storage.
- Provide predictable sample indexing.
- Preserve terrain dimensions and spatial metadata.
- Validate construction and mutation.
- Support efficient read-only access.
- Remain independent of rendering and graphics APIs.
- Be straightforward to test.

## 3. Namespace and Header

Namespace:

```cpp
dharaninirmana::terrain
```

Primary public header:

```text
include/
└── dharaninirmana/
    └── terrain/
        └── height_field.hpp
```

The implementation belongs to the Terrain module.

## 4. Coordinate Convention

DharaniNirmana uses a right-handed coordinate convention:

- X: Horizontal terrain axis.
- Y: Elevation.
- Z: Horizontal terrain axis.

The heightfield is sampled over the X-Z plane.

Each sample represents a world-space position:

```text
P(x, z) = (worldX, elevation, worldZ)
```

The initial grid increases along positive X and positive Z.

Triangle winding conventions are specified separately in [GEOMETRY_API.md](GEOMETRY_API.md).

## 5. TerrainOrigin

### 5.1 Purpose

The horizontal origin of a terrain is represented by a dedicated type.

Conceptual structure:

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
include/
└── dharaninirmana/
    └── terrain/
        └── terrain_origin.hpp
```

This type belongs to the Terrain domain because it expresses terrain-specific coordinate semantics.

### 5.2 Design Rationale

A generic `Vec2d` uses `x` and `y` coordinates.

Using it for terrain origin introduces ambiguity because the second horizontal coordinate is Z, not elevation Y.

TerrainOrigin makes this distinction explicit.

### 5.3 Requirements

- Both coordinates must be finite.
- The type follows value semantics.
- No graphics dependencies are permitted.
- The type must not contain elevation information.

## 6. HeightField Data Model

A HeightField consists of:

| Property | Type | Meaning |
|---|---|---|
| Width | size_t | Number of samples along X |
| Depth | size_t | Number of samples along Z |
| Spacing X | double | Distance between adjacent X samples |
| Spacing Z | double | Distance between adjacent Z samples |
| Origin | TerrainOrigin | Horizontal world-space origin |
| Heights | vector<double> | Elevation values |

The elevation storage is owned by the HeightField.

## 7. Memory Layout

The elevation array uses contiguous row-major storage.

For a heightfield with width W and depth D:

```text
sampleIndex = z * W + x
```

Valid coordinates:

```text
0 <= x < W
0 <= z < D
```

Example:

A heightfield with width 4 and depth 3:

```text
Z = 0: H00 H10 H20 H30
Z = 1: H01 H11 H21 H31
Z = 2: H02 H12 H22 H32
```

Linear storage:

```text
H00 H10 H20 H30 H01 H11 H21 H31 H02 H12 H22 H32
```

This layout provides contiguous storage and predictable indexing.

## 8. Spatial Mapping

For sample coordinates `(x, z)`:

```text
worldX = origin.x + x * spacingX
worldY = heights[z * width + x]
worldZ = origin.z + z * spacingZ
```

The resulting position is represented by:

```cpp
dharaninirmana::core::Vec3d
```

The implementation must ensure that coordinate calculations produce finite results.

A valid finite origin and spacing do not automatically guarantee finite world coordinates when dimensions are extremely large.

Spatial overflow must therefore be checked during construction or coordinate evaluation.

## 9. HeightField Public Interface

### 9.1 Construction

Conceptual API:

```cpp
static Result<HeightField> create(
    std::size_t width,
    std::size_t depth,
    double spacingX,
    double spacingZ,
    TerrainOrigin origin,
    std::vector<double> heights
);
```

Construction must validate all inputs before creating a valid object.

The factory returns a Result because dimensions, spacing, storage size, origin, and elevation values can be invalid.

The factory takes ownership of the supplied elevation storage.

### 9.2 Metadata Access

```cpp
std::size_t width() const noexcept;
std::size_t depth() const noexcept;

double spacingX() const noexcept;
double spacingZ() const noexcept;

TerrainOrigin origin() const noexcept;

std::size_t sampleCount() const noexcept;
```

These operations must not modify terrain.

### 9.3 Sample Access

```cpp
Result<double> heightAt(
    std::size_t x,
    std::size_t z
) const;
```

This operation returns an error for out-of-range coordinates.

### 9.4 Sample Mutation

```cpp
Result<void> setHeight(
    std::size_t x,
    std::size_t z,
    double height
);
```

This operation must reject:

- Out-of-range coordinates.
- Non-finite elevation values.

Invalid input must not modify terrain.

### 9.5 Position Access

```cpp
Result<Vec3d> samplePosition(
    std::size_t x,
    std::size_t z
) const;
```

This operation calculates the world-space position of a sample.

It must reject out-of-range coordinates and non-finite calculated positions.

## 10. Raw Sample Access

Algorithms requiring efficient access to all elevation samples may use:

```cpp
std::span<const double> heights() const noexcept;
```

This view:

- Does not transfer ownership.
- Does not allow mutation.
- Remains valid only while the owning HeightField and its underlying storage remain unchanged.
- Must not be retained beyond the documented lifetime.

Avoid exposing mutable storage directly.

Bulk mutation belongs to explicitly defined operations.

## 11. Dimension and Storage Validation

Construction must enforce the following invariants.

### 11.1 Dimensions

- Width must be at least 2.
- Depth must be at least 2.
- Width multiplied by depth must not overflow `std::size_t`.
- Elevation storage size must equal `width * depth`.

A minimum of two samples per axis is required because the initial Mesh Builder needs at least one terrain quad.

### 11.2 Spacing

- Spacing X must be finite and strictly positive.
- Spacing Z must be finite and strictly positive.

### 11.3 Origin

- Origin X must be finite.
- Origin Z must be finite.

### 11.4 Elevation

- Every elevation value must be finite.
- NaN and positive or negative infinity are invalid.

### 11.5 Spatial Extent

The calculated last sample coordinates must remain finite:

```text
lastX = origin.x + (width - 1) * spacingX

lastZ = origin.z + (depth - 1) * spacingZ
```

Construction should reject a terrain whose spatial extent cannot be represented with finite double values.

### 11.6 Failure

Invalid construction must return a descriptive Error.

A partially initialized HeightField must never be observable.

## 12. Ownership and Lifetime

HeightField exclusively owns its elevation storage.

The initial design follows value semantics.

Expected behavior:

- Copying a HeightField creates independent sample storage.
- Moving a HeightField transfers its owned storage.
- Destroying a HeightField releases its owned storage.
- Read-only spans do not extend the lifetime of the underlying storage.

No shared ownership is required initially.

## 13. Mutation and Consistency

Individual sample mutation is supported through `setHeight()`.

The operation must validate the new value before modifying storage.

A successful mutation changes only the specified elevation sample.

The following metadata remains unchanged:

- Width.
- Depth.
- Spacing.
- Origin.

Bulk processing operations belong to the Processing module.

The HeightField must not contain processing algorithms.

## 14. Terrain Bounds

The horizontal extent between the first and last samples is:

```text
extentX = (width - 1) * spacingX

extentZ = (depth - 1) * spacingZ
```

The horizontal bounds are:

```text
minX = origin.x
maxX = origin.x + extentX

minZ = origin.z
maxZ = origin.z + extentZ
```

The vertical bounds are determined by the minimum and maximum elevation samples.

A future bounds API may expose:

- Minimum elevation.
- Maximum elevation.
- Horizontal extent.
- Three-dimensional axis-aligned bounds.

These are deferred until required.

## 15. Error Handling

Terrain Data operations use Core Result and Error types.

Expected failures include:

| Operation | Failure |
|---|---|
| create | Invalid dimensions |
| create | Dimension multiplication overflow |
| create | Storage size mismatch |
| create | Invalid spacing |
| create | Non-finite origin |
| create | Non-finite elevation |
| create | Non-finite spatial extent |
| heightAt | Out-of-range coordinate |
| setHeight | Out-of-range coordinate |
| setHeight | Non-finite elevation |
| samplePosition | Out-of-range coordinate |
| samplePosition | Non-finite calculated position |

The exact error code must use the categories defined in [CORE_API.md](CORE_API.md).

## 16. Testing Requirements

### Construction Tests

- Valid terrain creation.
- Minimum valid dimensions.
- Invalid dimensions.
- Dimension multiplication overflow.
- Incorrect storage size.
- Invalid spacing.
- Non-finite origin.
- Non-finite elevations.
- Non-finite spatial extent.

### Indexing Tests

- First sample.
- Last sample.
- Interior sample.
- X-axis boundary.
- Z-axis boundary.
- Out-of-range X.
- Out-of-range Z.

### Mutation Tests

- Successful mutation.
- Out-of-range mutation.
- Non-finite elevation rejection.
- Invalid mutation leaves the original value unchanged.

### Spatial Mapping Tests

- Correct world X.
- Correct elevation Y.
- Correct world Z.
- Non-zero TerrainOrigin.
- Different X and Z spacing.
- Spatial overflow handling.

### Ownership Tests

- Copy independence.
- Move behavior.
- Read-only sample access.
- Span lifetime documentation.

All Terrain Data tests must run without a graphics context.

## 17. Initial Implementation Files

The initial Terrain module should contain:

```text
include/dharaninirmana/terrain/
├── terrain_origin.hpp
└── height_field.hpp

src/terrain/
└── height_field.cpp

tests/terrain/
└── height_field_tests.cpp
```

TerrainOrigin may be header-only.

Additional files should be introduced only when justified.

## 18. Future Extensions

The initial design should leave room for:

- Heightfield serialization.
- Terrain metadata.
- Multiple elevation layers.
- Heightfield sampling and interpolation.
- Non-uniform terrain transformations.
- Alternative terrain representations.

These extensions must not complicate the initial implementation unnecessarily.

## 19. Completion Criteria

The Terrain Data API is ready for implementation when:

- TerrainOrigin is defined.
- HeightField ownership is explicit.
- Memory layout is defined.
- Coordinate conventions are explicit.
- Construction invariants are documented.
- Sample access and mutation contracts are defined.
- Spatial mapping and overflow behavior are documented.
- Error behavior is specified.
- Independent tests are identified.
