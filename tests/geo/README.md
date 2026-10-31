---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Geo tests

This directory contains focused geo tests. It has no separate server
executable; tests are compiled as part of the parent ThemisDB build.

## Registration

`tests/geo/CMakeLists.txt` globs
`test_*.cpp` and registers one focused executable and CTest entry per source.
For example, `test_geo_json_geometry.cpp` maps to target
`module_geo_test_geo_json_geometry_focused` and CTest name
`test_geo_json_geometry_GeoFocusedTests`. The additional `test_geo_*.cpp`
registration loop also creates `_autofocused` targets/CTest entries when the
target is not already present.

From an already configured build tree with tests enabled, the example above is:

```sh
cmake --build <build-dir> --target module_geo_test_geo_json_geometry_focused
ctest --test-dir <build-dir> -R '^test_geo_json_geometry_GeoFocusedTests$' --output-on-failure
```

The command is an example of target naming, not evidence that the suite was
built or run. CUDA/HIP tests require the corresponding build/runtime to provide
GPU execution evidence; CPU fallback is not GPU validation. The contract
hardening test uses self-contained mock fixtures and is not a substitute for
testing concrete parsers, backends, indexes, or query implementations.

This CMake registration guide describes the integrated geo build. The optional
external Geo plugin may have a separate test setup; its test compatibility was
not verified in this sync.

## Test areas

The source set includes focused suites for GeoJSON/EWKB, geometry validation,
backend dispatch and device detection, indexes/cursors, joins/filters,
clustering, raster, temporal-spatial queries, tiles, and server integration.
Consult the individual test source and its CMake-generated target for exact
coverage and prerequisites.
