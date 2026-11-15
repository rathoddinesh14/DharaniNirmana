
# Generation API

## 1. Purpose

The Generation module provides algorithms for creating terrain data.

It defines a consistent interface for configuring generators, requesting terrain generation, validating parameters, and returning generated terrain.

The initial implementation focuses on procedural height-field generation using value noise.

The module depends on Core and Terrain and must remain independent of Geometry and Rendering.

## 2. Namespace and File Organization

The Generation module uses the `dharaninirmana::generation` namespace.

Its initial files are organized as follows:

```text
.
├── include/dharaninirmana/generation/
│   ├── generation_parameters.hpp
│   ├── value_noise_parameters.hpp
│   └── value_noise_generator.hpp
├── src/generation/
│   └── value_noise_generator.cpp
└── tests/generation/
    └── value_noise_generator_tests.cpp
```

Public headers define the generation request, value-noise configuration, and generator API. Implementation details belong under `src/`, and Generation tests belong under `tests/`.

## 3. Design Principles

The Generation API follows these principles:

- **Explicit configuration:** Algorithm-specific parameters are supplied when a generator is constructed.
- **Separate generation requests:** Output dimensions and spatial properties are supplied for each generation request.
- **Determinism:** Identical configurations and generation requests must produce identical results.
- **Ownership:** Generated terrain owns its data independently of the generator.
- **Validation:** Invalid or unsupported inputs must produce explicit errors.
- **Numerical reliability:** Floating-point calculations and coordinate conversions must be checked for invalid or unrepresentable results.

## 4. Generation Model

Generation separates two categories of parameters.

### 4.1 Algorithm Configuration

Algorithm-specific parameters configure the generator.

For example, value noise configuration may include:

- Frequency.
- Amplitude.
- Seed-related configuration, if not supplied through the generation request.
- Other algorithm-specific controls.

These parameters are supplied during generator construction.

The generator retains its configuration for subsequent generation requests.

### 4.2 Generation Request

Generation parameters describe the terrain to be produced.

The initial `GenerationParameters` abstraction contains:

| Parameter | Meaning |
|---|---|
| Width | Number of samples along X |
| Depth | Number of samples along Z |
| Spacing X | Distance between adjacent samples along X |
| Spacing Z | Distance between adjacent samples along Z |
| Origin | World-space location of the first sample |
| Seed | Seed controlling deterministic random variation |

The generation request must not contain algorithm-specific configuration.

This separation allows the same generator configuration to be reused for different terrain dimensions, locations, and seeds.

## 5. Core Interfaces

### 5.1 Generation Parameters

Conceptual declaration:

```cpp
namespace dharaninirmana::generation {

struct GenerationParameters {
    std::size_t width;
    std::size_t depth;

    double spacingX;
    double spacingZ;

    terrain::TerrainOrigin origin;

    std::uint64_t seed;
};
}
```

The declaration must use the terrain origin type and coordinate conventions defined in [TERRAIN_DATA_API.md](TERRAIN_DATA_API.md).

### 5.2 Value Noise Configuration

The value noise algorithm has a dedicated configuration type.

Conceptually:

```cpp
namespace dharaninirmana::generation {

struct ValueNoiseParameters {
    // Algorithm-specific configuration.
};
}
```

The configuration type must contain only parameters relevant to the value noise algorithm.

It must not duplicate output dimensions, terrain origin, or sample spacing from `GenerationParameters`.

### 5.3 Value Noise Generator

Conceptual interface:

```cpp
namespace dharaninirmana::generation {

class ValueNoiseGenerator {
public:
    static core::Result<ValueNoiseGenerator>
    create(const ValueNoiseParameters& parameters);

    core::Result<terrain::HeightField>
    generate(const GenerationParameters& parameters) const;

private:
    explicit ValueNoiseGenerator(
        const ValueNoiseParameters& parameters
    );

    ValueNoiseParameters parameters_;
};
}
```

The generator is created through a validating factory.

Invalid algorithm configuration must not produce a usable generator.

The generator owns its configuration and does not retain references to caller-owned configuration objects.

The `generate` operation is logically const and does not modify the generator configuration.

## 6. Generator Construction

The `create` operation validates algorithm-specific configuration.

Validation includes:

- All floating-point parameters must be finite.
- Parameters requiring positive values must be strictly positive.
- Parameters requiring non-negative values must not be negative.
- Integer parameters must satisfy documented algorithm limits.
- Combinations of parameters must remain within supported numerical limits.

The exact validation rules depend on the algorithm.

If configuration is invalid, the factory returns a failure result with a useful diagnostic.

A successfully constructed generator must satisfy all documented configuration invariants.

## 7. Generation Request Validation

Before allocating or generating terrain, the generator must validate the request.

### 7.1 Dimensions

- Width must be at least 2.
- Depth must be at least 2.
- The product `width * depth` must not overflow `std::size_t`.
- The resulting allocation size must be supported by the implementation and available resources.

Dimension validation must occur before calculating or allocating the elevation buffer.

### 7.2 Spacing

- `spacingX` must be finite and strictly positive.
- `spacingZ` must be finite and strictly positive.

Zero, negative, NaN, and infinite spacing values are invalid.

### 7.3 Origin

Both origin coordinates must be finite.

The origin represents the world-space location of the first sample.

### 7.4 Spatial Extent

The last sample coordinates must be representable and finite.

For a terrain with width $W$ and depth $D$:

$$
x_{\max} = origin_x + (W-1)\,spacingX
$$

$$
z_{\max} = origin_z + (D-1)\,spacingZ
$$

The implementation must verify that these calculations do not overflow or produce non-finite values.

A request that produces an invalid spatial extent must fail before generation begins.

## 8. Coordinate Mapping

Generation algorithms operate on a regular terrain grid.

For sample coordinates $(i,j)$:

$$
worldX = origin_x + i\,spacingX
$$

$$
worldZ = origin_z + j\,spacingZ
$$

The generated elevation is stored at:

$$
index = j \cdot width + i
$$

The resulting HeightField follows the coordinate convention:

- X: Horizontal position.
- Z: Horizontal position.
- Y: Elevation.

The same coordinate mapping must be used consistently by generation, terrain processing, and geometry construction.

## 9. Value Noise Evaluation

Value noise evaluates a continuous field using values associated with integer lattice coordinates.

A conceptual evaluation follows these steps:

1. Transform the input world coordinates into noise-space coordinates.
2. Identify the surrounding lattice points.
3. Retrieve deterministic pseudo-random values at those lattice points.
4. Interpolate the values.
5. Apply the configured amplitude and other algorithm-specific transformations.

The implementation must produce deterministic lattice values.

The lattice value must depend only on the documented seed and lattice coordinates.

It must not depend on:

- Memory addresses.
- Uninitialized memory.
- Thread scheduling.
- Non-deterministic global random state.

## 10. Numerical Robustness

Numerical reliability is part of the public behavior of the generation API.

### 10.1 Finite Input Validation

All floating-point inputs used by generation must be validated.

The implementation must reject:

- NaN.
- Positive infinity.
- Negative infinity.
- Invalid spacing.
- Non-finite origin.
- Non-finite algorithm parameters.

### 10.2 Lattice Coordinate Conversion

Value noise commonly transforms continuous coordinates into integer lattice coordinates using floor operations.

For a continuous noise-space coordinate $u$:

$$
i = \lfloor u \rfloor
$$

The conversion from floating-point coordinates to integer lattice coordinates must be checked.

The implementation must not perform an unchecked floating-point-to-integer conversion when the value is outside the representable range.

The implementation must also ensure that obtaining the neighboring lattice coordinate does not overflow the integer type.

If a coordinate cannot be represented safely, generation must fail with a numerical-range error.

The implementation must not silently wrap, clamp, or substitute an arbitrary lattice coordinate.

### 10.3 Noise-Space Coordinate Calculation

The transformation from world coordinates to noise-space coordinates must be evaluated safely.

For example, if frequency is applied to world coordinates:

$$
u = worldX \cdot frequency
$$

$$
v = worldZ \cdot frequency
$$

The implementation must check that the resulting coordinates are finite and within the supported lattice-coordinate range.

A finite world coordinate does not guarantee that its transformed noise-space coordinate is representable.

### 10.4 Interpolation

Interpolation must avoid unnecessary overflow.

The implementation should prefer numerically stable formulations and validate intermediate results where overflow is possible.

Interpolation must not silently convert invalid intermediate values into apparently valid terrain elevations.

### 10.5 Elevation Output

Every generated elevation must be finite.

If any sample becomes NaN or infinity, generation must fail rather than returning a partially valid HeightField.

The generator must not return a successful result containing invalid elevation samples.

### 10.6 Intermediate Arithmetic

The implementation must consider overflow and underflow in intermediate calculations, not merely validate the final result.

Where an intermediate calculation can overflow despite a mathematically representable final result, use a numerically safer formulation where practical.

The supported numerical range must be documented.

## 11. Determinism

Generation must be reproducible.

For a fixed combination of:

- Generator configuration.
- Generation parameters.
- Seed.
- Implementation version and documented numerical behavior.

The generator must produce the same elevation samples.

The implementation must use deterministic pseudo-random lattice values.

If parallel generation is introduced, the output must remain independent of execution order and thread scheduling.

Cross-platform bitwise identity is not guaranteed unless the implementation explicitly adopts and tests a cross-platform numerical reproducibility policy.

## 12. Memory and Ownership

The generated HeightField owns its elevation storage.

The generator must not return a HeightField that references:

- Temporary buffers.
- Generator-internal mutable storage.
- Caller-owned memory.
- Reused scratch memory.

The generated result must remain valid after the generator is destroyed.

Generation must either complete successfully with a fully valid HeightField or return a failure result.

No partially initialized terrain object may escape through a successful result.

## 13. Error Handling

The public generation API returns:

```cpp
core::Result<terrain::HeightField>
```

Expected failure categories include:

| Category | Example |
|---|---|
| Invalid configuration | Non-finite frequency |
| Invalid dimensions | Width or depth below 2 |
| Invalid spacing | Zero or negative spacing |
| Invalid origin | Non-finite coordinate |
| Size overflow | Width multiplied by depth overflows |
| Spatial overflow | Last sample coordinate is not finite |
| Numerical range | Noise-space coordinate exceeds supported lattice range |
| Numerical failure | Non-finite intermediate or output |
| Resource failure | Allocation cannot be completed |

Errors should identify the failed operation and provide useful diagnostic context.

The implementation must not silently change invalid inputs to make generation succeed.

## 14. Thread Safety

A successfully constructed generator should be safe for concurrent generation requests when its implementation uses only immutable configuration and request-local state.

The implementation must not use shared mutable scratch buffers or unsynchronized global random state.

If an implementation cannot satisfy concurrent use, that limitation must be explicitly documented.

## 15. Testing Requirements

The Generation module must include tests for:

### 15.1 Configuration

- Valid configuration.
- Invalid floating-point parameters.
- Boundary values.
- Unsupported parameter combinations.

### 15.2 Generation Requests

- Minimum valid dimensions.
- Invalid dimensions.
- Dimension multiplication overflow.
- Invalid spacing.
- Invalid origin.
- Spatial extent overflow.

### 15.3 Numerical Behavior

- Large world coordinates.
- Noise-space coordinate overflow.
- Lattice-coordinate range boundaries.
- Neighboring lattice-coordinate overflow.
- Non-finite intermediate values.
- Non-finite elevation output.

### 15.4 Determinism

- Same configuration and seed produce identical results.
- Different seeds produce different results for representative test inputs.
- Repeated generation does not depend on previous requests.
- Concurrent generation produces the same results as sequential generation.

### 15.5 Ownership

- Generated terrain remains valid after generator destruction.
- Generated terrain owns its elevation storage.
- Failed generation does not expose partially initialized output.

## 16. Future Extensions

The Generation module may eventually support:

- Gradient noise.
- Fractal Brownian motion.
- Domain warping.
- Ridged multifractal noise.
- Cellular noise.
- Hybrid procedural terrain algorithms.
- Parallel generation.
- GPU-assisted generation.

New algorithms must follow the same separation between algorithm configuration and generation requests unless a documented algorithm-specific requirement justifies a different contract.

## 17. Related Documents

- [API_DESIGN.md](API_DESIGN.md) — Public API conventions.
- [CORE_API.md](CORE_API.md) — Core types and Result semantics.
- [TERRAIN_DATA_API.md](TERRAIN_DATA_API.md) — HeightField representation.
- [PROCESSING_API.md](PROCESSING_API.md) — Terrain transformation operations.
- [GEOMETRY_API.md](GEOMETRY_API.md) — Geometry construction.
- [DEPENDENCY_RULES.md](../architecture/DEPENDENCY_RULES.md) — Module dependencies.
