# Geo Module - Future Enhancements

## Scope

This document covers planned enhancements to the Geospatial module beyond what is tracked in `ROADMAP.md`. It focuses on `cpu_backend.cpp`, `boost_cpu_exact_backend.cpp`, and `gpu_backend_stub.cpp`. Features here describe the engineering work required to complete full GeoJSON spec coverage, add CUDA kernel dispatch to the GPU backend, and introduce new spatial operations (ST_BUFFER, ST_UNION, spatial JOIN) that are currently absent from the module.

## Design Constraints

- The CPU-fallback path through `boost_cpu_exact_backend.cpp` must remain the authoritative reference implementation; GPU results are validated against it in tests.
- The circuit-breaker fallback mechanism in `gpu_backend_stub.cpp` must never be bypassed; any CUDA kernel failure must drop through to CPU automatically and record a structured audit-log entry.
- S2 and H3 cell indexing APIs must stay stable across GeoJSON spec changes; internal geometry representation changes are hidden behind the backend interface.
- No geometry input (GeoJSON payload, WKT string) may trigger arbitrary code execution; all parsers must validate and reject malformed input before passing geometry to backend operations.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `GeoBackend::stBuffer(geom, distance_m)` | AQL `GEO_BUFFER()` function, spatial JOIN planner | New operation; must be implemented in both `cpu_backend.cpp` and the GPU path |
| `GeoBackend::stUnion(geom_a, geom_b)` | AQL `GEO_UNION()` function | CPU-only initially; GPU deferred to v2.2.0 |
| `GeoJsonParser::parseGeometryCollection()` | `cpu_backend.cpp`, `boost_cpu_exact_backend.cpp` | Currently incomplete; must cover all seven GeoJSON geometry types per RFC 7946 |
| `SpatialIndex::rTreeQuery(bbox)` | `cpu_backend.cpp` query planner | New R-tree index replaces linear scan for contains/intersects on large datasets |
| `GpuKernelDispatcher::dispatch(op, geom[], n)` | `gpu_backend_stub.cpp` | New CUDA kernel entry point; stub to be replaced with real kernel in v2.1.0 |

## Planned Features

### Full GeoJSON Spec Coverage (RFC 7946)
**Priority:** High
**Target Version:** v1.6.0

Complete the GeoJSON parser to handle all seven geometry types: `Point`, `MultiPoint`, `LineString`, `MultiLineString`, `Polygon`, `MultiPolygon`, and `GeometryCollection`. The current implementation in `cpu_backend.cpp` handles `Point` and `Polygon` reliably; `MultiPolygon` and `GeometryCollection` are partially parsed and may silently drop rings or members.

**Implementation Notes:**
- Extend the GeoJSON parser in `cpu_backend.cpp` to validate the `type` discriminator before dispatching; add a `ParseError` enum with distinct codes for unknown type, invalid coordinates array, and out-of-range longitude/latitude.
- `GeometryCollection` must recursively parse member geometries up to a configurable nesting depth (default 8) to prevent stack overflow on adversarial input.
- Stricter parsing will reject previously accepted malformed inputs; document this as a breaking change in the changelog and add a `GEO_COMPAT_LAX` build flag for a one-release migration window.
- Add golden-file unit tests in `tests/geo/` covering each geometry type against the RFC 7946 example payloads.

**Performance Targets:**
- Parse 100 000 `MultiPolygon` features (average 20 rings each) in ≤ 2 s on a single core.
- Zero allocations per coordinate after initial parse (use `std::span` views over the parsed coordinate array).

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

### ST_BUFFER Operation
**Priority:** Medium
**Target Version:** v1.7.0

Implement `ST_BUFFER(geometry, distance_meters)` which expands a geometry by a fixed geodesic distance. This is required by several planned AQL spatial query patterns (proximity search, corridor queries) and is listed as a blocker in the geo module roadmap.

**Implementation Notes:**
- Implement in `cpu_backend.cpp` using `boost::geometry::buffer` with a point-circle strategy for geodesic accuracy at the target distance scales (1 m – 100 km).
- For polygons, use the `join_round` and `end_round` Boost.Geometry strategies with a configurable point count per arc (default 36; user-overridable via `GeoConfig::buffer_arc_points`).
- Add AQL binding `GEO_BUFFER(doc.location, 500)` (distance in metres) documented in `docs/aql/geo_functions.md`.
- GPU path deferred: initial release is CPU-only with a note in `gpu_backend_stub.cpp` marking `stBuffer` as unsupported; it falls back to CPU automatically via the circuit breaker.

**Performance Targets:**
- Buffer 10 000 `Point` geometries at 500 m radius in ≤ 200 ms on a single core.
- Output polygon vertex count ≤ 200 per buffered point at default arc resolution.

---

### CUDA Kernel Dispatch for GPU Backend
**Priority:** Medium
**Target Version:** v2.1.0

Replace the CPU-fallback stub in `gpu_backend_stub.cpp` with real CUDA kernels for `contains`, `intersects`, and `distance` operations on `Point` arrays. The GPU backend targets datasets where N > 1 M points and latency requirements preclude CPU processing.

**Implementation Notes:**
- Add `gpu_backend_cuda.cu` with a `__global__` kernel for batch point-in-polygon using the ray-casting algorithm; one CUDA thread per query point.
- Device memory management via the existing `gpu_memory_manager.cpp` in `src/llm/`; introduce a shared `GpuMemoryPool` abstraction that both modules can register with.
- On `cudaErrorNoDevice` or any CUDA runtime error, set the circuit-breaker flag in `gpu_backend_stub.cpp` and log a structured entry with `backend=gpu`, `op=contains`, `error=<cudaGetErrorString>`.
- Add a CMake option `THEMIS_GEO_CUDA=ON` gated on `CMAKE_CUDA_COMPILER` being found; default OFF so CPU-only builds are unaffected.

**Performance Targets:**
- Batch `contains` query (1 M points, 1 polygon) completes in ≤ 50 ms on an NVIDIA A10G (vs ~4 s on a single CPU core).
- GPU memory allocation for a 1 M point batch ≤ 32 MB device memory.

---

### Spatial JOIN Support
**Priority:** Low
**Target Version:** v2.2.0

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
| ST_BUFFER, 10 K points at 500 m | N/A | ≤ 200 ms | New benchmark in `benchmarks/geo_bench.cpp` |
| GPU contains, 1 M points (A10G) | N/A (CPU fallback) | ≤ 50 ms | CUDA kernel benchmark gated on `THEMIS_GEO_CUDA=ON` |

## Security / Reliability

- All GeoJSON inputs must be validated for coordinate range (longitude ∈ [−180, 180], latitude ∈ [−90, 90]) before being passed to any backend; out-of-range values are rejected with a typed error, not silently clamped.
- `GeometryCollection` recursive parsing is bounded by a configurable depth limit (default 8) to prevent stack-overflow attacks from deeply nested inputs.
- CUDA kernel failures always trigger the circuit-breaker and fall back to CPU; partial GPU results are never returned to the caller.
- The structured audit log entry for every GPU↔CPU backend switch must include timestamp, operation name, error code, and collection ID to support incident investigation.
