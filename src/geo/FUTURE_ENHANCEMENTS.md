# Geo Module - Future Enhancements

## Scope

This document covers planned enhancements to the Geospatial module beyond what is tracked in `ROADMAP.md`. It focuses on `cpu_backend.cpp`, `boost_cpu_exact_backend.cpp`, and `gpu_backend_stub.cpp`. Note: the following features are now fully implemented — ST_BUFFER, spatial JOIN, ST_UNION, ST_DIFFERENCE, full GeoJSON RFC 7946 spec coverage, CUDA kernel dispatch, and raster data queries (elevation, heatmaps). Items listed below without an ✅ status are still planned.

## Design Constraints

- The CPU-fallback path through `boost_cpu_exact_backend.cpp` must remain the authoritative reference implementation; GPU results are validated against it in tests.
- The circuit-breaker fallback mechanism in `gpu_backend_stub.cpp` must never be bypassed; any CUDA kernel failure must drop through to CPU automatically and record a structured audit-log entry.
- S2 and H3 cell indexing APIs must stay stable across GeoJSON spec changes; internal geometry representation changes are hidden behind the backend interface.
- No geometry input (GeoJSON payload, WKT string) may trigger arbitrary code execution; all parsers must validate and reject malformed input before passing geometry to backend operations.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `GeoBackend::stBuffer(geom, distance_m)` | AQL `GEO_BUFFER()` function, spatial JOIN planner | New operation; must be implemented in both `cpu_backend.cpp` and the GPU path |
| `GeoBackend::stUnion(geom_a, geom_b)` | AQL `ST_UNION()` function | **Implemented**: CPU-exact (Greiner-Hormann), Boost (`union_`), GPU CPU-fallback; CUDA kernel deferred to v2.2.0 |
| `GeoBackend::stDifference(geom_a, geom_b)` | AQL `ST_DIFFERENCE()` function | **Implemented**: CPU-exact (Greiner-Hormann), Boost (`difference`), GPU CPU-fallback; CUDA kernel deferred to v2.2.0 |
| `GeoJsonParser::parseGeometryCollection()` | `cpu_backend.cpp`, `boost_cpu_exact_backend.cpp` | Currently incomplete; must cover all seven GeoJSON geometry types per RFC 7946 |
| `SpatialIndex::rTreeQuery(bbox)` | `cpu_backend.cpp` query planner | New R-tree index replaces linear scan for contains/intersects on large datasets |
| `GpuKernelDispatcher::dispatch(op, geom[], n)` | `gpu_backend_stub.cpp` | New CUDA kernel entry point; stub to be replaced with real kernel in v2.1.0 |

## Planned Features

### Full GeoJSON Spec Coverage (RFC 7946)
**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented in `src/geo/cpu_backend.cpp` and `src/geo/boost_cpu_exact_backend.cpp`

All seven geometry types (`Point`, `MultiPoint`, `LineString`, `MultiLineString`, `Polygon`, `MultiPolygon`, `GeometryCollection`) are handled. `GeometryCollection` is parsed recursively. Strict coordinate-range validation rejects out-of-range WGS84 values; a `THEMIS_GEO_COMPAT_LAX` build flag provides a one-release migration window.

---

### R-tree Spatial Index for CPU Backend
**Priority:** High
**Target Version:** v1.6.0

Replace the linear scan in `boost_cpu_exact_backend.cpp` for `contains` and `intersects` queries with an in-memory R-tree index (Boost.Geometry `rtree` with `rstar` packing algorithm). For collections with > 10 000 geometries the current O(n) scan becomes the dominant query cost.

**Implementation Notes:**
- Add `SpatialIndex` class in a new `spatial_index.cpp`; wrap `boost::geometry::index::rtree<value, bgi::rstar<16>>`.
- Index is built lazily on first spatial query and cached per collection handle; invalidated on write.
- Expose `SpatialIndex::bulkLoad(geometries)` for cold-start performance (STR packing is 3–5× faster than incremental insert for read-heavy workloads).
- Index memory usage must be reported via the existing structured audit log (`geo_index_bytes_allocated` field) so operators can observe RSS growth.

**Performance Targets:**
- `intersects` query over 1 M point geometries: ≤ 5 ms p99 with R-tree vs ~2 s with linear scan.
- Bulk-load of 1 M geometries into the R-tree ≤ 3 s wall clock.

---

### ST_BUFFER Operation ✅ Implemented
**Target Version:** v1.7.0 — **Status: Complete**

`ST_BUFFER(geometry, distance_meters)` expands a geometry by a fixed geodesic distance.  Both the `cpu_backend.cpp` (exact) and `boost_cpu_exact_backend.cpp` (Boost.Geometry) backends implement `ISpatialComputeBackend::stBuffer()`; the GPU backend delegates to the CPU path with an audit-log entry pending a future CUDA kernel (v2.1.0).

**What was implemented:**
- `CpuExactBackend::stBuffer()` — geodesic-aware, latitude-based degree conversion:
  - Point → circular polygon ring (`arc_points` vertices, default 36, minimum 3)
  - Polygon → outward ring expansion via edge-shift + adjacent-edge intersection
- `BoostCpuExactBackend::stBuffer()` — uses `bg::buffer` with `join_round`/`end_round` strategies for smooth output; converts metres to degrees at the geometry's centroid latitude
- `GpuBatchBackend::stBuffer()` — CPU fallback with audit log + GPU metrics counter
- AQL binding `ST_Buffer(geom, distance)` in `let_evaluator.cpp` and `query_engine.cpp` (MVP cartesian: Point → square, Polygon → MBR expansion)
- 9 parametric unit tests (×2 backends) in `tests/geo/test_geo_st_buffer.cpp`; 2 AQL-layer tests in `tests/geo/test_aql_st_functions.cpp`

**Performance Targets (design):**
- Buffer 10 000 `Point` geometries at 500 m radius in ≤ 200 ms on a single core.
- Output polygon vertex count ≤ 200 per buffered point at default arc resolution.

---

### CUDA Kernel Dispatch for GPU Backend
**Priority:** Medium
**Target Version:** v2.1.0
**Status:** ✅ Implemented in `src/geo/gpu_backend_cuda.cu` and `src/acceleration/cuda/geo_kernels.cu`

Real CUDA kernels (`haversineDistanceKernel`, `pointInPolygonKernel`) are implemented with host↔device memory management via `GpuKernelDispatcher`. The CMake option `THEMIS_GEO_CUDA=ON` enables the CUDA path; CPU-only builds use the fallback path unchanged. Any CUDA runtime error triggers the circuit-breaker in `gpu_backend_stub.cpp` and logs a structured audit entry.

---

### Spatial JOIN Support
**Priority:** Low
**Target Version:** v2.2.0
**Status:** ✅ Implemented in `include/geo/spatial_join.h` + `src/geo/spatial_join.cpp`

Add a spatial JOIN operation that finds all pairs (A, B) from two geometry collections where `distance(A, B) ≤ threshold`. This enables nearest-neighbour and within-radius multi-collection queries from AQL.

**Implementation Notes:**
- Implement as a nested-loop join with R-tree index on the inner collection (`SpatialIndex::rTreeQuery(expandedBbox)`); avoids O(n²) brute-force for typical cardinalities.
- Add AQL syntax `FOR a IN colA FOR b IN colB FILTER GEO_DISTANCE(a.loc, b.loc) <= 1000 RETURN ...` via a new join rule in the AQL query optimizer.
- Expose join result as a lazy iterator to avoid materializing all pairs in memory; yield one pair at a time to the AQL execution engine.
- Add a configurable `max_pairs` limit (default 1 M) with a warning log when the limit is hit to prevent runaway queries.

**Performance Targets:**
- Spatial JOIN of two 100 000-point collections with 1 km threshold returns first 1 000 results in ≤ 500 ms.
- Memory usage during JOIN bounded by R-tree index size + O(batch_size) working set.

---

### Raster Data Query Support (Elevation, Heatmaps)
**Priority:** Low
**Target Version:** v2.3.0
**Status:** ✅ Implemented in `include/geo/raster.h` + `src/geo/raster.cpp`

Provide a lightweight, header-only-friendly raster abstraction for elevation maps, heatmaps, and any regularly-gridded scalar dataset referenced to geographic coordinates.

**What was implemented:**
- `RasterGrid` struct: 2D row-major grid of `float` values with WGS84 bounding box, configurable no-data sentinel (default: quiet NaN), and O(1) cell accessors `at()` / `set()`.
- `sampleAt(grid, lon, lat)` — bilinear interpolation with no-data cell exclusion and re-normalisation; returns a `RasterSampleResult{value, valid}` pair.
- `queryBBox(grid, bbox)` — copies the sub-raster whose cell centres fall inside `bbox` into a new `RasterGrid`; O(output cells) work with no allocations beyond the output buffer.
- `generateHeatmap(points, bbox, config)` — Gaussian kernel density estimator: bandwidth in metres converted to degrees at the grid's centre latitude; 3σ kernel radius cutoff; optional normalisation to [0, 1]; points outside `bbox` are skipped.

**Implementation Notes:**
- `RasterGrid` stores data independently of any geometry backend; it can be used without `ISpatialComputeBackend`.
- Bandwidth conversion uses the Haversine-consistent `metresToDegreesLon/Lat` helpers local to `raster.cpp` to avoid pulling in the full spatial backend at compile time.
- No dynamic allocation occurs inside `sampleAt` or `queryBBox` hot paths; the output vector for `queryBBox` is pre-sized once.

**Performance Targets:**
- `sampleAt` on a 1 M-cell grid: ≤ 1 µs per call (single core, no SIMD).
- `queryBBox` extracting 10 000 cells from a 1 M-cell grid: ≤ 10 ms.
- `generateHeatmap` of 100 000 points onto a 100×100 grid at 500 m bandwidth: ≤ 500 ms single-threaded.

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Each geometry type parser tested with RFC 7946 golden fixtures; `SpatialIndex` tested with known bbox queries; negative tests for malformed GeoJSON inputs |
| Integration | CPU backend + GPU fallback path | Tests must exercise the circuit-breaker: force `cudaErrorNoDevice` with a mock and assert fallback to `boost_cpu_exact_backend.cpp` with audit log entry |
| Performance | Throughput regression < 10% on CPU path | `benchmarks/geo_bench.cpp` runs in CI; R-tree query benchmark must show ≥ 100× speedup over linear scan at N=1 M |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| `intersects` query, 1 M points, linear scan | ~2 000 ms | ≤ 5 ms (R-tree) | `benchmarks/geo_bench.cpp`, 1 M point fixture |
| GeoJSON parse, 100 K MultiPolygon features | ~5 s (estimate) | ≤ 2 s | Parse-only benchmark in `tests/geo/bench_parse.cpp` |
| ST_BUFFER, 10 K points at 500 m | N/A (CPU) | ≤ 200 ms | New benchmark in `benchmarks/geo_bench.cpp` |
| GPU contains, 1 M points (A10G) | N/A (CPU fallback) | ≤ 50 ms | CUDA kernel benchmark gated on `THEMIS_GEO_CUDA=ON` |

## Security / Reliability

- All GeoJSON inputs must be validated for coordinate range (longitude ∈ [−180, 180], latitude ∈ [−90, 90]) before being passed to any backend; out-of-range values are rejected with a typed error, not silently clamped.
- `GeometryCollection` recursive parsing is bounded by a configurable depth limit (default 8) to prevent stack-overflow attacks from deeply nested inputs.
- CUDA kernel failures always trigger the circuit-breaker and fall back to CPU; partial GPU results are never returned to the caller.
- The structured audit log entry for every GPU↔CPU backend switch must include timestamp, operation name, error code, and collection ID to support incident investigation.
