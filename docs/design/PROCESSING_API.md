
# Processing API

## 1. Purpose

The Processing module provides operations that transform existing terrain data.

It operates on terrain representations without depending on the algorithms that originally generated them.

The initial implementation focuses on fundamental elevation-processing operations:

- Smoothing.
- Elevation normalization.

The module depends on Core and Terrain and must remain independent of Generation, Geometry, and Rendering.

## 2. Namespace and File Organization

The Processing module uses the `dharaninirmana::processing` namespace.

Its initial directory structure is:

```text
.
├── include/dharaninirmana/processing/
│   └── [operation-specific public headers]
├── src/processing/
│   └── [operation implementations]
└── tests/processing/
    └── [operation tests]
```

Exact filenames will be selected as the operation interfaces are finalized.

## 3. Design Principles

The Processing API follows these principles:

- **Explicit operations:** Each operation has a clearly defined input, output, and parameter contract.
- **Controlled mutation:** APIs must make it clear whether an operation modifies its input or creates a new result.
- **Data preservation:** Terrain dimensions and spatial metadata must remain unchanged unless explicitly documented otherwise.
- **Numerical robustness:** Intermediate calculations must be designed to avoid unnecessary overflow and underflow.
- **Validation:** Invalid parameters and invalid terrain data must produce explicit errors.
- **Composability:** Processing operations should be usable independently and combined into larger workflows.

## 4. Processing Model

Processing operations consume an existing HeightField and produce a transformed HeightField.

The initial API uses a non-mutating model:

```text
       Input HeightField
               |
               v
       Validate Input
               |
               v
       Processing Operation
               |
               v
       Validate Output
               |
               v
       Result<HeightField>
```

The input terrain remains unchanged.

The returned HeightField owns its elevation storage independently of the input.

## 5. Common Processing Contract

Every public processing operation must follow these rules:

1. Validate the input terrain.
2. Validate operation-specific parameters.
3. Check dimensions and allocation requirements.
4. Perform the transformation.
5. Validate the generated elevations.
6. Preserve terrain metadata.
7. Return an owned HeightField on success.
8. Return a meaningful error on failure.

A failed operation must not modify the input.

A successful operation must not contain non-finite elevation values.

## 6. Terrain Metadata Preservation

Unless an operation explicitly documents otherwise, processing must preserve:

- Width.
- Depth.
- Horizontal spacing along X.
- Horizontal spacing along Z.
- Terrain origin.

Only elevation values are transformed.

The output must maintain the same sample ordering and coordinate mapping as the input.

## 7. Smoothing

### 7.1 Purpose

Smoothing reduces local elevation variations by replacing each sample with a weighted combination of neighboring samples.

It can be used to reduce high-frequency noise and produce smoother terrain.

### 7.2 Initial Algorithm

The initial implementation uses a neighborhood-based weighted average.

For an interior sample at grid position $(i,j)$:

$$
h'_{i,j} =
\frac{
\sum_{(p,q)\in N(i,j)} w_{p,q}h_{p,q}
}{
\sum_{(p,q)\in N(i,j)} w_{p,q}
}
$$

Where:

- $h_{p,q}$ is an input elevation.
- $w_{p,q}$ is a non-negative neighborhood weight.
- $N(i,j)$ is the selected neighborhood.
- $h'_{i,j}$ is the resulting elevation.

The initial implementation may use a uniform box neighborhood.

The exact neighborhood size and boundary policy must be exposed through documented processing parameters.

### 7.3 Boundary Policy

The initial smoothing implementation uses a truncated neighborhood at terrain boundaries.

Only valid samples inside the HeightField contribute to the result.

The normalization weight is calculated from the samples actually included.

This avoids requiring samples outside the terrain extent.

### 7.4 Parameter Validation

Smoothing parameters must satisfy their documented constraints.

For the initial implementation:

- Neighborhood radius must be non-negative.
- Neighborhood dimensions must be representable.
- Weight values must be finite and non-negative.
- At least one positive weight must contribute to each output sample.

Unsupported or invalid parameter combinations must return an error.

## 8. Numerically Stable Smoothing

A mathematically valid average can overflow when implemented using a naive accumulation strategy.

For example:

```cpp
sum += weight * elevation;
```

Even when the final weighted average is representable, the intermediate sum may overflow.

The implementation must not rely solely on checking whether the final result is finite.

### 8.1 Stable Weighted Average

For non-negative weights, use a numerically stable weighted-mean strategy.

One suitable approach is:

1. Find the maximum absolute elevation magnitude among the contributing samples.
2. If that magnitude is zero, return zero.
3. Scale each elevation by the maximum magnitude.
4. Accumulate the scaled weighted values.
5. Accumulate the weights using a safe strategy.
6. Divide the scaled weighted sum by the total weight.
7. Rescale the result.
8. Verify that the final elevation is finite.

Conceptually:

$$
M = \max_k |h_k|
$$

For $M > 0$:

$$
\mu =
M \cdot
\frac{
\sum_k w_k(h_k/M)
}{
\sum_k w_k
}
$$

This avoids multiplying large elevations directly by weights.

The implementation must also ensure that the accumulated weights cannot overflow.

For a fixed small neighborhood, this may be achieved through bounded weights or normalized weights.

For more general neighborhoods, use a stable accumulation strategy.

### 8.2 Zero Magnitude

If all contributing elevations are zero:

$$
M = 0
$$

The resulting weighted average is zero.

The implementation must handle this case without division by zero.

### 8.3 Intermediate Validation

The implementation must validate:

- Scaled elevations.
- Weight normalization.
- Accumulated weighted values.
- Final rescaled elevations.

If a result cannot be represented as a finite `double`, smoothing must fail.

### 8.4 Iterative Smoothing

If multiple iterations are supported, each iteration must operate on a well-defined intermediate buffer.

The implementation must not update samples in-place in a way that causes the result to depend unintentionally on traversal order.

A failed iteration must not expose partially processed output.

## 9. Elevation Normalization

### 9.1 Purpose

Normalization transforms terrain elevations into a specified target range.

Given the input elevation range:

$$
[h_{\min}, h_{\max}]
$$

and target range:

$$
[t_{\min}, t_{\max}]
$$

the conventional linear transformation is:

$$
h' =
t_{\min}
+
\frac{h-h_{\min}}{h_{\max}-h_{\min}}
(t_{\max}-t_{\min})
$$

However, this direct expression is not sufficiently robust for all finite `double` values.

The implementation must use an overflow-aware formulation.

### 9.2 Parameter Validation

The normalization operation must validate:

- Target minimum is finite.
- Target maximum is finite.
- Target minimum is less than or equal to target maximum.
- Input elevations are finite.

A constant input field must be handled explicitly.

### 9.3 Constant Input Field

If:

$$
h_{\min} = h_{\max}
$$

the input has no elevation variation.

The implementation must not divide by zero.

The initial policy maps every sample to the target minimum.

This behavior must be documented and tested.

## 10. Numerically Stable Normalization

### 10.1 Avoid Direct Range Subtraction

For finite extreme values, the following expression may overflow:

```cpp
double range = maximum - minimum;
```

For example, subtracting a large negative finite value from a large positive finite value may produce infinity.

The implementation must not assume that finite inputs guarantee finite intermediate arithmetic.

### 10.2 Stable Input Scaling

A suitable approach is to select the normalization formula based on the signs of the input bounds.

### Same-Sign Bounds

When both bounds are non-negative or both are non-positive, direct subtraction is safe because the input interval does not cross zero.

Compute:

$$
u = \frac{h-h_{\min}}{h_{\max}-h_{\min}}
$$

Since all input samples lie within the specified interval, the differences remain representable for finite bounds of the same sign.

### Opposite-Sign Bounds

When the input interval crosses zero, direct subtraction may overflow.

Let:

$$
S = \max(|h_{\min}|, |h_{\max}|)
$$

For $S > 0$, compute:

$$
a = h_{\min}/S
$$

$$
b = h_{\max}/S
$$

For each sample:

$$
x = h/S
$$

Then compute:

$$
u = \frac{x-a}{b-a}
$$

This keeps the scaled bounds within $[-1,1]$ and avoids overflow during range subtraction.

### Validation

The implementation must verify that:

- The denominator is strictly positive.
- All intermediate values are finite.
- The normalized coordinate is within $[0,1]$, allowing for documented floating-point rounding.
- The final result is finite.

The case $S=0$ corresponds to a zero-valued constant field and is handled by the constant-field policy.

### 10.3 Target Range Mapping

The normalized coordinate $u$ lies in the interval $[0,1]$.

Use C++20 `std::lerp` for target-range interpolation:

$$
h' = \operatorname{lerp}(t_{\min}, t_{\max}, u)
$$

Conceptually:

```cpp
const double output = std::lerp(
    targetMinimum,
    targetMaximum,
    normalizedValue
);
```

### 10.4 Output Validation

Every normalized elevation must be finite.

If the requested target range or intermediate calculation cannot produce representable finite outputs, normalization must fail.

The implementation must not silently clamp results to the nearest finite value.

## 11. Numerical Tolerance

The initial smoothing and normalization operations do not use a geometric degeneracy tolerance.

Their numerical policies are based on representability and finite-value validation.

The implementation must not introduce arbitrary epsilon-based modifications to elevation values.

Tolerance policies for geometric calculations belong to the Geometry module.

## 12. Ownership and Memory

Processing operations return a new HeightField.

The result owns its elevation storage.

The input HeightField remains unchanged.

Temporary buffers must not escape through public APIs.

If an operation requires multiple working buffers, their allocation and lifetime must be controlled within the operation.

Allocation failure must be converted into an appropriate failure result where the project's error-handling policy permits it.

## 13. Error Handling

Public processing operations return:

```cpp
core::Result<terrain::HeightField>
```

Expected failure categories include:

| Category | Example |
|---|---|
| Invalid input | Invalid HeightField |
| Invalid parameter | Negative neighborhood radius |
| Invalid weight | Non-finite or negative weight |
| Invalid target range | Non-finite target bounds |
| Numerical overflow | Unrepresentable intermediate or output |
| Numerical failure | Non-finite generated elevation |
| Resource failure | Allocation failure |

Errors should identify the operation and the reason for failure.

A failed operation must never return a partially valid HeightField as a successful result.

## 14. Testing Requirements

### 14.1 Smoothing

Test:

- Uniform terrain.
- Single elevated sample.
- Boundary samples.
- Different neighborhood sizes.
- Zero-radius behavior.
- Multiple iterations.
- Negative elevations.
- Large finite elevations.
- Small finite elevations.
- Weighted averages with large weights.
- Constant terrain.
- Input immutability.

### 14.2 Normalization

Test:

- Positive-only elevations.
- Negative-only elevations.
- Mixed positive and negative elevations.
- Constant elevation fields.
- Zero-valued fields.
- Negative target ranges.
- Target ranges crossing zero.
- Extreme finite input elevations.
- Extreme finite target bounds.
- Invalid target bounds.
- Non-finite input values.

### 14.3 Numerical Properties

Verify:

- All successful output elevations are finite.
- Smoothing preserves constant fields.
- Smoothing produces values within the contributing neighborhood's minimum and maximum for non-negative weights.
- Normalization maps the input minimum to the target minimum.
- Normalization maps the input maximum to the target maximum.
- Normalized values remain within the target range, subject to floating-point rounding.
- Failed operations do not modify the input.

## 15. Future Extensions

The Processing module may eventually support:

- Gaussian smoothing.
- Median filtering.
- Bilateral filtering.
- Hydraulic erosion.
- Thermal erosion.
- Slope and curvature analysis.
- Terrain blending.
- Hydrological analysis.

Each operation must define its own mathematical contract, parameter validation, boundary policy, and numerical behavior.

## 16. Related Documents

- [API_DESIGN.md](API_DESIGN.md) — Public API conventions.
- [CORE_API.md](CORE_API.md) — Error handling and foundational types.
- [TERRAIN_DATA_API.md](TERRAIN_DATA_API.md) — HeightField representation.
- [GENERATION_API.md](GENERATION_API.md) — Procedural terrain generation.
- [GEOMETRY_API.md](GEOMETRY_API.md) — Geometry construction and numerical tolerances.
- [DEPENDENCY_RULES.md](../architecture/DEPENDENCY_RULES.md) — Module dependency constraints.
