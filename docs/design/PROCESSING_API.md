
# Processing API

## 1. Purpose

This document defines the public API for terrain processing in DharaniNirmana.

The Processing module provides operations that transform existing terrain data into new terrain representations.

Processing is separate from terrain generation.

Generation creates terrain from parameters. Processing modifies or derives terrain from existing terrain.

The module must remain independent of rendering and graphics APIs.

## 2. Design Goals

The Processing API must:

- Provide reusable terrain transformations.
- Support composition of multiple operations.
- Preserve terrain metadata unless explicitly documented otherwise.
- Make mutation behavior predictable.
- Validate operation parameters.
- Avoid hidden state.
- Support deterministic processing.
- Remain independently testable.

## 3. Namespace and Header Organization

Namespace:

```cpp
dharaninirmana::processing
```

Initial public headers:

```text
include/dharaninirmana/processing/
├── terrain_operation.hpp
├── height_operations.hpp
└── smoothing_operation.hpp
```

Implementation:

```text
src/processing/
├── height_operations.cpp
└── smoothing_operation.cpp
```

Tests:

```text
tests/processing/
├── height_operations_tests.cpp
└── smoothing_operation_tests.cpp
```

## 4. Processing Model

### 4.1 Input and Output

The initial processing model is:

```text
HeightField
    |
    v
Processing Operation
    |
    v
New HeightField
```

Operations receive a read-only input and produce a new output.

Conceptual contract:

```cpp
Result<HeightField> operation(
    const HeightField& input
) const;
```

This prevents accidental mutation of the source terrain.

### 4.2 Immutable-by-Default Policy

The initial public processing API must not mutate its input.

Benefits:

- Predictable behavior.
- Easier unit testing.
- Safe reuse of source terrain.
- Straightforward operation composition.
- Reduced risk of partial mutation after failure.

In-place processing may be considered later if profiling demonstrates a meaningful performance requirement.

It must be introduced explicitly rather than silently changing existing operation semantics.

## 5. Terrain Operation Interface

The initial architecture supports a common operation contract.

Conceptual interface:

```cpp
class ITerrainOperation {
public:
    virtual ~ITerrainOperation() = default;

    virtual Result<HeightField> apply(
        const HeightField& input
    ) const = 0;
};
```

The interface represents a transformation from one valid HeightField to another.

Not every operation needs to use this interface. Simple free functions are acceptable where they produce a clearer API.

Avoid introducing excessive inheritance solely for uniformity.

## 6. Initial Operations

The first implementation will provide five operations.

| Operation | Purpose |
|---|---|
| Height Scaling | Multiply elevations by a factor |
| Height Offset | Add a constant elevation offset |
| Height Clamping | Restrict elevations to a range |
| Height Normalization | Map elevation range into a target range |
| Smoothing | Reduce local elevation variation |

Each operation must preserve dimensions, spacing, and origin.

## 7. Height Scaling

### Responsibility

Multiply every elevation sample by a specified scale factor.

Conceptual transformation:

```text
outputHeight = inputHeight * scale
```

### Parameters

```cpp
struct HeightScaleParameters {
    double scale;
};
```

### Validation

- Scale must be finite.
- Negative scale values are permitted because vertical inversion may be useful.
- Every resulting elevation must remain finite.

### API

```cpp
Result<HeightField> scaleHeights(
    const HeightField& input,
    const HeightScaleParameters& parameters
);
```

### Example

Input:

```text
[1, 2, 3]
```

Scale:

```text
2.0
```

Output:

```text
[2, 4, 6]
```

## 8. Height Offset

### Responsibility

Add a constant value to every elevation sample.

Transformation:

```text
outputHeight = inputHeight + offset
```

### Parameters

```cpp
struct HeightOffsetParameters {
    double offset;
};
```

### Validation

- Offset must be finite.
- Resulting elevation values must remain finite.

### API

```cpp
Result<HeightField> offsetHeights(
    const HeightField& input,
    const HeightOffsetParameters& parameters
);
```

### Example

Input:

```text
[1, 2, 3]
```

Offset:

```text
10.0
```

Output:

```text
[11, 12, 13]
```

## 9. Height Clamping

### Responsibility

Restrict elevation values to a specified interval.

Transformation:

```text
outputHeight = clamp(
    inputHeight,
    minimum,
    maximum
)
```

### Parameters

```cpp
struct HeightClampParameters {
    double minimum;
    double maximum;
};
```

### Validation

- Minimum and maximum must be finite.
- Minimum must not exceed maximum.

### API

```cpp
Result<HeightField> clampHeights(
    const HeightField& input,
    const HeightClampParameters& parameters
);
```

### Example

Input:

```text
[-5, 2, 15]
```

Range:

```text
[0, 10]
```

Output:

```text
[0, 2, 10]
```

## 10. Height Normalization

### Responsibility

Map the input terrain's elevation range into a specified target range.

Transformation:

```text
normalizedHeight =
    targetMinimum +
    ((inputHeight - inputMinimum) /
     (inputMaximum - inputMinimum)) *
    (targetMaximum - targetMinimum)
```

### Parameters

```cpp
struct HeightNormalizationParameters {
    double targetMinimum;
    double targetMaximum;
};
```

### Validation

- Target bounds must be finite.
- Target minimum must not exceed target maximum.
- Input elevation values must be finite.

### Constant Terrain Policy

If all input elevations are equal:

```text
inputMinimum == inputMaximum
```

The normalization formula would divide by zero.

The initial policy must handle this explicitly.

For constant terrain, assign every output sample to the midpoint of the target range:

```text
outputHeight =
    (targetMinimum + targetMaximum) / 2
```

The implementation must calculate this safely to avoid overflow.

### API

```cpp
Result<HeightField> normalizeHeights(
    const HeightField& input,
    const HeightNormalizationParameters& parameters
);
```

## 11. Smoothing

### Responsibility

Reduce local elevation variation by applying a neighborhood filter.

The initial implementation will use a simple box filter.

### Parameters

```cpp
struct SmoothingParameters {
    std::uint32_t radius;
    std::uint32_t iterations;
};
```

### Neighborhood

For each sample, inspect neighboring samples within the configured radius.

The output elevation is the arithmetic mean of the selected neighborhood.

Conceptually:

```text
outputHeight(x, z) =
    sum(neighborHeights) /
    neighborCount
```

### Boundary Policy

The initial implementation uses a truncated neighborhood.

Samples outside the terrain boundary are ignored.

The average is calculated using only valid neighboring samples.

This avoids introducing artificial values beyond the terrain boundary.

### Validation

- Radius must be greater than zero.
- Iterations must be greater than zero.
- Iteration count must remain within a documented implementation limit.
- Intermediate results must remain finite.

### API

```cpp
Result<HeightField> smoothTerrain(
    const HeightField& input,
    const SmoothingParameters& parameters
);
```

### Iteration Semantics

Each iteration operates on the result of the previous iteration.

The original input remains unchanged.

## 12. Metadata Preservation

All initial processing operations must preserve:

- Width.
- Depth.
- Spacing X.
- Spacing Z.
- Origin.

Only elevation samples may change.

Future operations that modify spatial metadata must explicitly document their behavior.

## 13. Operation Composition

Operations should be composable.

Example:

```text
Generated Terrain
      |
      v
  Scale Heights
      |
      v
  Smooth Terrain
      |
      v
  Clamp Heights
      |
      v
Processed Terrain
```

The initial API does not require a dedicated processing pipeline framework.

Applications may compose operations through ordinary function calls.

A pipeline abstraction can be introduced later if it solves a concrete usability or performance problem.

## 14. Error Handling

Processing functions return:

```cpp
Result<HeightField>
```

Expected failures include:

| Condition | Error Category |
|---|---|
| Invalid scale | InvalidArgument |
| Invalid offset | InvalidArgument |
| Invalid clamp range | InvalidArgument |
| Invalid normalization range | InvalidArgument |
| Invalid smoothing parameters | InvalidArgument |
| Invalid input terrain | InvalidData |
| Non-finite intermediate result | InvalidData |
| Allocation failure | ResourceExhausted |

Failure must not expose partially processed terrain.

The original input must remain unchanged after any failure.

## 15. Numerical Considerations

Processing algorithms must:

- Use double precision internally.
- Avoid unnecessary intermediate allocations.
- Validate results for finiteness.
- Use numerically safe accumulation.
- Avoid overflow where practical.
- Document behavior for extreme input ranges.

For smoothing, the implementation must use an accumulation strategy appropriate for the supported elevation range.

The initial implementation should prioritize correctness over aggressive optimization.

## 16. Performance Considerations

The initial implementation will be synchronous.

Requirements:

- Allocate output storage once where practical.
- Traverse samples in row-major order.
- Avoid unnecessary copies.
- Reuse temporary buffers between smoothing iterations where appropriate.
- Do not introduce multithreading before profiling.

Performance optimizations must preserve documented numerical and behavioral contracts.

## 17. Testing Requirements

### Height Scaling

- Positive scale.
- Zero scale.
- Negative scale.
- Invalid non-finite scale.
- Overflow detection.

### Height Offset

- Positive offset.
- Negative offset.
- Zero offset.
- Non-finite offset.
- Overflow detection.

### Height Clamping

- Values below minimum.
- Values above maximum.
- Values within range.
- Equal minimum and maximum.
- Invalid range.

### Height Normalization

- Normal terrain range.
- Negative elevation range.
- Target range with negative values.
- Constant terrain.
- Invalid target range.
- Finite output guarantees.

### Smoothing

- Radius of one.
- Multiple iterations.
- Boundary behavior.
- Constant terrain preservation.
- Smallest valid terrain.
- Invalid radius.
- Invalid iteration count.
- Finite output guarantees.

### General Processing

- Source terrain remains unchanged.
- Metadata is preserved.
- Output dimensions are preserved.
- Processing results can be composed.
- Invalid operations return errors.
- Failure does not expose partial output.

All Processing tests must run without a graphics context.

## 18. Initial Implementation Files

The initial Processing module should contain:

```text
include/dharaninirmana/processing/
├── height_operations.hpp
└── smoothing_operation.hpp

src/processing/
├── height_operations.cpp
└── smoothing_operation.cpp

tests/processing/
├── height_operations_tests.cpp
└── smoothing_operation_tests.cpp
```

A shared operation interface should be added only if multiple concrete use cases justify it.

## 19. Future Extensions

Potential future operations:

- Gaussian smoothing.
- Median filtering.
- Terrain erosion.
- Hydraulic erosion.
- Thermal erosion.
- Slope calculation.
- Curvature calculation.
- Heightfield resampling.
- Terrain cropping.
- Terrain blending.

These are outside the initial implementation scope.

## 20. Completion Criteria

The Processing API is ready for implementation when:

- Mutation policy is explicit.
- Initial operations are defined.
- Parameters and validation rules are documented.
- Boundary behavior is specified.
- Metadata preservation is guaranteed.
- Error handling is defined.
- Numerical considerations are documented.
- Independent tests are identified.
