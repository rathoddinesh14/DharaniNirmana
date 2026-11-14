
# Generation API

## 1. Purpose

This document defines the public API for procedural terrain generation in DharaniNirmana.

The Generation module transforms generation parameters into a valid Terrain Data representation.

Its responsibilities include:

- Defining generator interfaces.
- Defining common terrain generation parameters.
- Managing deterministic randomness.
- Validating algorithm-specific configuration.
- Producing HeightField instances.
- Supporting multiple generation algorithms.

The Generation module must not depend on graphics APIs.

## 2. Design Goals

The Generation API must:

- Separate generation algorithms from terrain storage.
- Support deterministic generation.
- Make algorithm configuration explicit.
- Produce valid HeightField instances.
- Allow new algorithms without modifying existing terrain data structures.
- Remain independently testable.
- Avoid global random state.
- Separate common terrain configuration from algorithm-specific configuration.

## 3. Namespace and Header Organization

Namespace:

```cpp
dharaninirmana::generation
```

Initial public headers:

```text
include/dharaninirmana/generation/
├── generation_parameters.hpp
├── terrain_generator.hpp
└── value_noise_generator.hpp
```

Implementation:

```text
src/generation/
└── value_noise_generator.cpp
```

Tests:

```text
tests/generation/
└── value_noise_generator_tests.cpp
```

## 4. Generation Architecture

The Generation module consists of:

| Component | Responsibility |
|---|---|
| GenerationParameters | Defines common terrain dimensions and spatial configuration |
| ITerrainGenerator | Defines the common generation contract |
| ValueNoiseParameters | Defines value-noise-specific configuration |
| ValueNoiseGenerator | Implements the first procedural algorithm |

The generator is configured when it is created.

Generation calls supply only terrain-level parameters.

Conceptual pipeline:

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

## 5. Generation Parameters

### 5.1 Common Parameters

GenerationParameters defines properties shared by terrain generation algorithms.

Conceptual structure:

```cpp
struct GenerationParameters {
    std::size_t width;
    std::size_t depth;

    double spacingX;
    double spacingZ;

    TerrainOrigin origin;

    std::uint64_t seed;
};
```

The type belongs to:

```cpp
dharaninirmana::generation
```

It depends on the Terrain module for TerrainOrigin.

### 5.2 Parameter Semantics

| Parameter | Meaning |
|---|---|
| width | Number of samples along X |
| depth | Number of samples along Z |
| spacingX | Distance between adjacent X samples |
| spacingZ | Distance between adjacent Z samples |
| origin | Horizontal world-space origin |
| seed | Deterministic random seed |

The generated HeightField must preserve the supplied dimensions, spacing, and origin.

### 5.3 Validation

Before generation begins, validate:

- Width and depth are at least 2.
- Width multiplied by depth does not overflow.
- Spacing values are finite and positive.
- Origin coordinates are finite.
- Calculated spatial extents are finite.
- Requested allocation is representable and feasible.

Invalid parameters must return an explicit Error.

## 6. Generator Interface

### 6.1 ITerrainGenerator

All terrain generators follow a common interface.

Conceptual API:

```cpp
class ITerrainGenerator {
public:
    virtual ~ITerrainGenerator() = default;

    virtual Result<HeightField> generate(
        const GenerationParameters& parameters
    ) const = 0;
};
```

The interface defines only the common generation operation.

Algorithm-specific configuration does not belong in GenerationParameters unless it genuinely applies to all algorithms.

### 6.2 Configuration Ownership

Each concrete generator owns its algorithm-specific configuration.

Configuration is supplied when creating the generator instance.

The generator retains its configuration for its lifetime.

The `generate()` operation receives only common terrain parameters.

This separation provides:

- A stable common generator interface.
- Explicit algorithm configuration.
- Reusable configured generator instances.
- No algorithm-specific parameter leakage into other generators.

### 6.3 Stateless Generation

After construction, generators should behave as immutable algorithm configurations.

Generation must not modify the stored algorithm configuration.

Randomness must be derived from the seed supplied through GenerationParameters.

This permits repeated calls using the same generator configuration.

Independent calls must not depend on previous calls.

## 7. First Algorithm: Value Noise

The first implementation uses two-dimensional value noise.

Value noise is selected because it is:

- Relatively simple to implement.
- Deterministic.
- Suitable for continuous terrain elevation.
- Useful for validating the generation architecture.
- Easy to extend through multiple octaves.

This implementation establishes the generation pipeline rather than targeting production-quality landscapes.

## 8. Value Noise Configuration

### 8.1 ValueNoiseParameters

The algorithm-specific configuration is:

```cpp
struct ValueNoiseParameters {
    double frequency;
    double amplitude;
    std::uint32_t octaves;
    double persistence;
    double lacunarity;
};
```

This structure belongs to:

```cpp
dharaninirmana::generation
```

It is independent of terrain dimensions, origin, spacing, and seed.

### 8.2 Parameter Definitions

| Parameter | Meaning |
|---|---|
| frequency | Initial spatial frequency |
| amplitude | Initial elevation amplitude |
| octaves | Number of noise layers |
| persistence | Amplitude multiplier between octaves |
| lacunarity | Frequency multiplier between octaves |

### 8.3 Validation

Configuration must satisfy:

- Frequency is finite and positive.
- Amplitude is finite and non-negative.
- Octaves are at least 1.
- Octave count does not exceed the documented implementation limit.
- Persistence is finite and non-negative.
- Lacunarity is finite and positive.
- Derived frequencies and amplitudes remain representable.
- Intermediate calculations must not silently overflow.

The maximum octave count must be selected and documented during implementation.

## 9. ValueNoiseGenerator Construction

### 9.1 Factory API

The generator is created using a validated factory.

Conceptual API:

```cpp
class ValueNoiseGenerator final : public ITerrainGenerator {
public:
    static Result<ValueNoiseGenerator> create(
        const ValueNoiseParameters& parameters
    );

    Result<HeightField> generate(
        const GenerationParameters& parameters
    ) const override;

private:
    explicit ValueNoiseGenerator(
        ValueNoiseParameters parameters
    );

    ValueNoiseParameters parameters_;
};
```

The constructor is private.

Applications must use the factory to create a configured generator.

### 9.2 Construction Behavior

`create()` must:

1. Validate all ValueNoiseParameters.
2. Validate derived configuration values where practical.
3. Return an error if configuration is invalid.
4. Construct the generator only after successful validation.

The resulting generator owns a copy of its configuration.

Its configuration cannot be modified through the public API.

### 9.3 Usage Example

Conceptual usage:

```text
ValueNoiseParameters noiseParameters{
    .frequency = 0.01,
    .amplitude = 100.0,
    .octaves = 5,
    .persistence = 0.5,
    .lacunarity = 2.0
};

auto generatorResult =
    ValueNoiseGenerator::create(noiseParameters);

if (!generatorResult) {
    reportError(generatorResult.error());
    return;
}

auto generator = std::move(generatorResult).value();

GenerationParameters terrainParameters{
    .width = 512,
    .depth = 512,
    .spacingX = 1.0,
    .spacingZ = 1.0,
    .origin = TerrainOrigin{0.0, 0.0},
    .seed = 12345
};

auto terrainResult =
    generator.generate(terrainParameters);
```

This demonstrates the separation between algorithm configuration and terrain configuration.

The example is conceptual; exact initialization and error-reporting helpers will be finalized during implementation.

## 10. Noise Algorithm

### 10.1 Lattice Values

Value noise operates over a two-dimensional integer lattice.

Each lattice point receives a deterministic pseudo-random value derived from:

- Integer lattice coordinates.
- Generation seed.
- Octave index.

The same coordinates, seed, and octave must always produce the same value.

No global random number generator is permitted.

### 10.2 Interpolation

For a noise coordinate:

```text
(x, z)
```

Find the four surrounding lattice points:

```text
(x0, z0)
(x1, z0)
(x0, z1)
(x1, z1)
```

Calculate fractional coordinates:

```text
tx = x - x0
tz = z - z0
```

Use the quintic fade function:

```text
fade(t) = 6t^5 - 15t^4 + 10t^3
```

Interpolate horizontally between lattice values, then vertically between the resulting values.

This produces continuous noise across lattice cell boundaries.

### 10.3 Octave Combination

For octave index i:

```text
octaveFrequency = frequency * lacunarity^i

octaveAmplitude = amplitude * persistence^i
```

The final elevation is:

```text
height(x, z) =
    sum(
        octaveAmplitude[i] *
        noise(
            x * octaveFrequency[i],
            z * octaveFrequency[i],
            seed,
            i
        )
    )
```

The implementation must use numerically safe accumulation.

Normalization is deferred.

The initial implementation must document the resulting elevation range and the effect of configuration parameters.

## 11. Coordinate Sampling

Noise coordinates are based on world-space horizontal positions.

For a HeightField sample:

```text
worldX = origin.x + x * spacingX

worldZ = origin.z + z * spacingZ
```

The generator evaluates noise at these coordinates.

Frequency determines the spatial scale of the generated pattern.

Generation must respect the configured world-space origin and spacing.

## 12. Determinism Contract

The complete generation configuration consists of:

- Generator algorithm.
- Algorithm-specific configuration.
- Common terrain parameters.
- Random seed.

For identical configurations and inputs, repeated generation must produce identical terrain.

The output must have identical:

- Dimensions.
- Spacing.
- TerrainOrigin.
- Elevation samples.

Generation must not depend on:

- Global random state.
- Current time.
- Uninitialized memory.
- Previous generator calls.
- Thread scheduling.
- Unspecified iteration order.

The initial guarantee applies to the same implementation and supported numerical environment.

Bitwise-identical output across different compilers and processor architectures is not guaranteed until explicitly validated.

## 13. Error Handling

Generator creation returns:

```cpp
Result<ValueNoiseGenerator>
```

Generation returns:

```cpp
Result<HeightField>
```

Expected failures include:

| Condition | Error Category |
|---|---|
| Invalid frequency | InvalidArgument |
| Invalid amplitude | InvalidArgument |
| Invalid octave count | InvalidArgument |
| Invalid persistence | InvalidArgument |
| Invalid lacunarity | InvalidArgument |
| Invalid terrain dimensions | InvalidArgument |
| Invalid spacing | InvalidArgument |
| Invalid origin | InvalidArgument |
| Dimension overflow | InvalidArgument |
| Non-finite intermediate data | InvalidData |
| Allocation failure | ResourceExhausted |

The generator must not return partially initialized terrain.

Allocation failures should be translated into the documented result error where practical and supported by the project's exception policy.

## 14. Memory and Performance

The initial generator produces a contiguous HeightField.

Requirements:

- Allocate elevation storage once where practical.
- Avoid unnecessary temporary arrays.
- Use predictable iteration order.
- Keep noise evaluation independent of graphics resources.
- Avoid premature parallelization.
- Reuse immutable algorithm configuration across generation calls.

The initial implementation prioritizes correctness and deterministic behavior.

Performance optimization will follow profiling.

## 15. Testing Requirements

### Configuration Validation

- Valid configuration.
- Invalid frequency.
- Invalid amplitude.
- Invalid octave count.
- Invalid persistence.
- Invalid lacunarity.
- Excessive octave count.
- Invalid derived frequency or amplitude.

### Construction

- Successful factory creation.
- Invalid configuration returns an error.
- Generator retains its configuration.
- No invalid generator can be constructed through the public API.

### Generation Parameters

- Minimum valid dimensions.
- Invalid dimensions.
- Invalid spacing.
- Invalid origin.
- Spatial extent overflow.
- Dimension multiplication overflow.

### Determinism

- Same configuration and seed produce identical terrain.
- Different seeds produce different terrain for a suitable non-degenerate configuration.
- Repeated calls on the same generator produce identical results.
- Independent generator instances with identical configuration produce identical results.

### Spatial Properties

- Correct dimensions.
- Correct spacing.
- Correct TerrainOrigin.
- Correct sample count.
- Correct world-space sampling behavior.

### Noise Properties

- Deterministic lattice values.
- Continuity across lattice boundaries.
- Correct interpolation.
- Correct octave combination.
- Finite output values.

### Failure Behavior

- Invalid configuration returns an error.
- Invalid generation parameters return an error.
- Failed generation does not expose partial terrain.
- Allocation failures follow the documented error policy.

All Generation tests must run without a graphics context.

## 16. Initial Implementation Files

The initial Generation module should contain:

```text
include/dharaninirmana/generation/
├── generation_parameters.hpp
├── terrain_generator.hpp
└── value_noise_generator.hpp

src/generation/
└── value_noise_generator.cpp

tests/generation/
└── value_noise_generator_tests.cpp
```

Additional private files may be introduced for hashing, interpolation, and noise evaluation when implementation complexity justifies separation.

## 17. Future Extensions

The architecture should allow future algorithms such as:

- Fractal Brownian motion variations.
- Gradient noise.
- Simplex noise.
- Ridged multifractal noise.
- Domain warping.
- Hybrid terrain generation.
- Erosion-based generation.

Each algorithm should own its own configuration and implement the common generator contract when it produces the same HeightField representation.

## 18. Design Decisions Deferred

The following details will be finalized during implementation:

- Exact hash or pseudo-random function.
- Exact lattice value range.
- Noise normalization strategy.
- Maximum supported octave count.
- Allocation size policy.
- Numerical tolerances.
- Exact derived-value validation strategy.

The construction model and separation between common and algorithm-specific parameters are architectural decisions and must not be changed casually.

## 19. Completion Criteria

The Generation API is ready for implementation when:

- Common generation parameters are defined.
- Generator interface is established.
- Algorithm-specific configuration is separate.
- Generator construction validates configuration.
- Generation calls receive only common parameters.
- First algorithm is selected.
- Determinism requirements are explicit.
- Validation rules are defined.
- Error behavior is documented.
- Independent tests are identified.
