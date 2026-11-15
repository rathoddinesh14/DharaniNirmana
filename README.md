# DharaniNirmana

**A modular C++20 terrain generation engine.**

DharaniNirmana is a graphics-independent terrain engine designed around modular architecture, reusable mathematical foundations, validated terrain data structures, and extensible terrain-generation capabilities.

The project separates terrain data, generation algorithms, processing, geometry, and rendering integration into independent modules.

## Project Goals

- **Modularity:** Keep engine responsibilities separated into well-defined modules.
- **Graphics Independence:** Terrain data and algorithms must not depend on graphics APIs.
- **Mathematical Foundation:** Use [Aryabhatta](https://github.com/rathoddinesh14/Aryabhatta) for mathematical types and operations.
- **Rendering Integration:** Keep rendering optional through [Vishwakarma](https://github.com/rathoddinesh14/vishwakarma).
- **Correctness:** Validate inputs and expose predictable error-handling interfaces.
- **Extensibility:** Support future terrain-generation algorithms and geometry-processing capabilities.

## Technology Stack

| Component | Technology |
|---|---|
| Language | C++20 |
| Build system | CMake 3.25+ |
| Compiler | GCC 10+ recommended |
| Mathematics | Aryabhatta |
| Testing | CTest |
| Rendering integration | Vishwakarma (planned) |

## Architecture

The engine is organized into the following modules:

```text
DharaniNirmana
│
├── Core
│   ├── Error handling
│   ├── Result<T>
│   └── Common engine utilities
│
├── Terrain
│   ├── TerrainOrigin
│   └── HeightField
│
├── Generation
│   └── Terrain generation algorithms
│
├── Processing
│   └── Terrain processing operations
│
├── Geometry
│   └── Terrain geometry and mesh representation
│
└── Rendering (Optional)
    └── Vishwakarma integration
```

### Module Dependencies

```text
             Core
               │
             Terrain
          ┌────┼────┐
          │    │    │
     Generation │  Geometry
               │
           Processing

Rendering (Optional)
    └── Core + Geometry + Vishwakarma
```

Rendering is optional and disabled by default.

### CMake Targets

| Module | CMake target |
|---|---|
| Core | `DharaniNirmana::Core` |
| Terrain | `DharaniNirmana::Terrain` |
| Generation | `DharaniNirmana::Generation` |
| Processing | `DharaniNirmana::Processing` |
| Geometry | `DharaniNirmana::Geometry` |
| Rendering | `DharaniNirmana::Rendering` (planned) |

## Mathematical Foundation

DharaniNirmana uses Aryabhatta for mathematical types.

Public APIs use the mathematical types directly:

```cpp
aryabhatta::math::DVec2
aryabhatta::math::DVec3
```

The engine does not introduce duplicate vector abstractions.

Terrain-specific coordinate semantics are represented by `TerrainOrigin`, which explicitly stores horizontal X and Z coordinates. Y represents elevation.

Repository: https://github.com/rathoddinesh14/Aryabhatta

## Coordinate Convention

The engine follows a right-handed coordinate convention:

| Axis | Meaning |
|---|---|
| X | Horizontal terrain axis |
| Y | Elevation |
| Z | Horizontal terrain axis |

Terrain is sampled over the X-Z plane.

A terrain sample represents:

```text
P(x, z) = (worldX, elevation, worldZ)
```

## Current Implementation

### Core

- Error representation through `Error` and `ErrorCode`.
- Generic `Result<T>` and `Result<void>` types.
- Explicit success and failure construction.
- Move-only value support.
- Checked access to result values and errors.

### Terrain

**TerrainOrigin**

- Represents the horizontal world-space origin.
- Stores X and Z coordinates.
- Uses value semantics.

**HeightField**

- Represents terrain using a regular two-dimensional elevation grid.
- Owns contiguous elevation storage.
- Uses row-major indexing:

```cpp
sampleIndex = z * width + x;
```

- Validates dimensions, spacing, origin, elevation values, and spatial extent.
- Provides checked sample access and mutation.
- Provides world-space sample positions using `aryabhatta::math::DVec3`.
- Exposes read-only elevation access through `std::span<const double>`.
- Supports independent copy and move semantics.

## Build Instructions

### Requirements

- CMake 3.25 or later.
- GCC 10 and G++ 10.
- Git.
- Aryabhatta, either installed or fetched through CMake.
- GLM, required by Aryabhatta.

### Configure

Use GCC 10 explicitly:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=gcc-10 \
  -DCMAKE_CXX_COMPILER=g++-10 \
  -DDHARANINIRMANA_BUILD_TESTS=ON \
  -DDHARANINIRMANA_BUILD_RENDERING=OFF \
  -DDHARANINIRMANA_FETCH_ARYABHATTA=ON
```

### Build

```bash
cmake --build build --parallel
```

### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

### Clean Build

If changing compilers or resetting the build configuration:

```bash
rm -rf build
```

Then configure and build again.

## Testing

The project uses CTest to execute module-level tests.

Current test suites:

| Test suite | Coverage |
|---|---|
| `DharaniNirmana.Core` | Error handling, Result types, Aryabhatta integration |
| `DharaniNirmana.Terrain` | HeightField construction, validation, indexing, mutation, spatial mapping, ownership |

Latest local verification:

- Core tests: Passed.
- Terrain tests: Passed.
- Total: 2/2 test suites passed.

## Repository Structure

```text
DharaniNirmana/
│
├── CMakeLists.txt
│
├── include/
│   └── dharaninirmana/
│       ├── core/
│       └── terrain/
│
├── src/
│   └── terrain/
│
├── tests/
│   ├── core/
│   └── terrain/
│
└── docs/
    ├── architecture/
    └── design/
```

## Related Projects

- **Aryabhatta:** Mathematical foundation library — https://github.com/rathoddinesh14/Aryabhatta
- **Vishwakarma:** Graphics library for future rendering integration — https://github.com/rathoddinesh14/vishwakarma

## Development Principles

- Keep Core and Terrain independent of graphics APIs.
- Avoid unnecessary abstraction layers around Aryabhatta types.
- Validate public API inputs.
- Prefer explicit error handling over silent failure.
- Keep module dependencies aligned with the architecture.
- Add tests alongside new functionality.
- Keep optional rendering integration outside the core engine.

## License

MIT License.