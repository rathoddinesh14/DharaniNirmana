# Implementation Increment 1 — Manual Verification Checklist

This ZIP is an overlay for the existing DharaniNirmana repository. Extract its contents into the repository root; do not replace the existing `docs/`, `LICENSE`, or `.gitignore` directories/files.

## 1. Files added

- [ ] `CMakeLists.txt`
- [ ] `include/dharaninirmana/core/error_code.hpp`
- [ ] `include/dharaninirmana/core/error.hpp`
- [ ] `include/dharaninirmana/core/result.hpp`
- [ ] `tests/core/CMakeLists.txt`
- [ ] `tests/core/test_main.cpp`
- [ ] Empty module directory placeholders under `include/` and `src/` (tracked using `.gitkeep` files)

## 2. Configure and build

Run from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DDHARANINIRMANA_BUILD_TESTS=ON -DDHARANINIRMANA_BUILD_RENDERING=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

- [ ] CMake configures with the installed CMake version (3.25 or newer).
- [ ] Build completes with the configured C++20 compiler.
- [ ] CTest discovers `DharaniNirmana.Core`.
- [ ] All Core tests pass.

## 3. Verify target names and dependency direction

- [ ] `DharaniNirmana::Core` exists.
- [ ] `DharaniNirmana::Terrain` links to Core.
- [ ] `DharaniNirmana::Generation` links to Core and Terrain.
- [ ] `DharaniNirmana::Processing` links to Core and Terrain.
- [ ] `DharaniNirmana::Geometry` links to Core and Terrain.
- [ ] No core target links to Vishwakarma, OpenGL, GLFW, or Rendering.
- [ ] `DHARANINIRMANA_BUILD_RENDERING` defaults to `OFF`.

Optional inspection:

```bash
cmake --build build --target help
```

## 4. Verify Core API behavior

- [ ] `Error` owns its message and preserves its `ErrorCode`.
- [ ] `Result<T>` uses explicit `success(value)` and `failure(error)` factories.
- [ ] `Result<T>` reports state through `hasValue()` and explicit boolean conversion.
- [ ] `value()` works for success and throws `std::logic_error` on failure.
- [ ] `error()` works for failure and throws `std::logic_error` on success.
- [ ] `Result<T>` supports move-only payloads such as `std::unique_ptr`.
- [ ] `Result<void>` represents success with no error and failure with an `Error`.
- [ ] Invalid `Result<void>` access throws `std::logic_error`.

## 5. Verify graphics independence

- [ ] Configure/build succeeds without Vishwakarma installed or added as a subdirectory.
- [ ] No OpenGL or GLFW package discovery is performed by this root build.
- [ ] The rendering option remains OFF; do not enable it yet. Enabling it intentionally reports that Rendering is not implemented in this increment.

## 6. Review before committing

- [ ] Compare the extracted files with the existing repository; do not overwrite unrelated work.
- [ ] Check `git status --short` and confirm only the intended new files are present.
- [ ] Review the diff, especially the root `CMakeLists.txt` and public headers.
- [ ] Commit only after the checklist passes.

## Scope boundary

This increment establishes build structure and Core error/result primitives only. It does not implement mathematical types, HeightField, generation algorithms, processing algorithms, mesh types/builders, or rendering integration. Aryabhatta owns mathematical types and operations; engine APIs use those types directly. Other capabilities should be separate increments with focused tests.
