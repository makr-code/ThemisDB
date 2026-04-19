> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Geo Module - Future Enhancements

This document covers planned enhancements to the Geospatial module beyond what is tracked in `ROADMAP.md`. It focuses on `cpu_backend.cpp`, `boost_cpu_exact_backend.cpp`, and `gpu_backend_stub.cpp`. Note: the following features are now fully implemented — ST_BUFFER, spatial JOIN, ST_UNION, ST_DIFFERENCE, full GeoJSON RFC 7946 spec coverage, CUDA kernel dispatch, and raster data queries (elevation, heatmaps). Items listed below without an ✅ status are still planned.

## Design Constraints

- The CPU-fallback path through `boost_cpu_exact_backend.cpp` must remain the authoritative reference implementation; GPU results are validated against it in tests.
- The circuit-breaker fallback mechanism in `gpu_backend_stub.cpp` must never be bypassed; any CUDA kernel failure must drop through to CPU automatically and record a structured audit-log entry.
- S2 and H3 cell indexing APIs must stay stable across GeoJSON spec changes; internal geometry representation changes are hidden behind the backend interface.
- No geometry input (GeoJSON payload, WKT string) may trigger arbitrary code execution; all parsers must validate and reject malformed input before passing geometry to backend operations.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `GeoBackend::stBuffer(geom, distance_m)` | AQL `GEO_BUFFER()` function, spatial JOIN planner | **Implemented**: CPU-exact and Boost backends; GPU delegates to CPU (CUDA kernel deferred to v2.2.0) |
| `GeoBackend::stUnion(geom_a, geom_b)` | AQL `ST_UNION()` function | **Implemented**: CPU-exact (Greiner-Hormann), Boost (`union_`), GPU CPU-fallback; CUDA kernel deferred to v2.2.0 |
| `GeoBackend::stDifference(geom_a, geom_b)` | AQL `ST_DIFFERENCE()` function | **Implemented**: CPU-exact (Greiner-Hormann), Boost (`difference`), GPU CPU-fallback; CUDA kernel deferred to v2.2.0 |
| `GeoJsonParser::parseGeometryCollection()` | `cpu_backend.cpp`, `boost_cpu_exact_backend.cpp` | **Implemented**: all seven GeoJSON geometry types per RFC 7946 |
| `SpatialIndex::rTreeQuery(bbox)` | `cpu_backend.cpp` query planner | **Implemented**: R-tree index in `geo_rtree.cpp` replaces linear scan |
| `GpuKernelDispatcher::dispatch(op, geom[], n)` | `gpu_backend_stub.cpp` | **Implemented**: CUDA (`gpu_backend_cuda.cu`) and HIP (`gpu_backend_hip.cpp`) kernel dispatch |

## Planned Features

### CUDA and OpenCL Implementation in `gpu_backend_production.cpp`
**Priority:** High
**Target Version:** v1.4.0

`gpu_backend_production.cpp` has 2 explicit TODO comments:
- Line 262: `// TODO v1.4.0: Complete CUDA implementation with geometry data processing`
- Line 362: `// TODO v1.4.0: Complete OpenCL implementation`

Both GPU paths currently fall back to the CPU backend, making the "production" GPU backend no different from the CPU backend.

**Implementation Notes:**
- `[ ]` Implement CUDA geometry data processing at line 262: upload WGS-84 point arrays to device memory using `cudaMalloc`; dispatch the distance/containment kernels in `gpu/cuda_kernels.cu`; download results via `cudaMemcpy`.
- `[ ]` Implement OpenCL dispatch at line 362: compile geo kernels via `clCreateProgramWithSource` at startup; enqueue NDRange kernels for point-in-polygon and distance batch operations.
- `[ ]` Add CUDA/OpenCL parity tests: compare GPU and CPU results for 10 K point dataset; verify max absolute error ≤ 1 mm for WGS-84 distance.
- `[ ]` Register a `GpuBackendRegistry` entry (currently a "Simple internal registry stub" in `cpu_backend.cpp` line 914) so the production GPU backend is discoverable at runtime.

**Performance Targets:**
- GPU batch distance (1 M WGS-84 point pairs): ≥ 8× speedup vs. CPU `boost_cpu_exact_backend.cpp` on RTX-class hardware.
- GPU point-in-polygon (1 M points, 1 K polygon): ≥ 5× speedup vs. CPU.

---


**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented in `src/geo/cpu_backend.cpp` and `src/geo/boost_cpu_exact_backend.cpp`

All seven geometry types (`Point`, `MultiPoint`, `LineString`, `MultiLineString`, `Polygon`, `MultiPolygon`, `GeometryCollection`) are handled. `GeometryCollection` is parsed recursively. Strict coordinate-range validation rejects out-of-range WGS84 values; a `THEMIS_GEO_COMPAT_LAX` build flag provides a one-release migration window.

---

### R-tree Spatial Index for CPU Backend
**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented in `src/geo/geo_rtree.cpp` + `include/geo/geo_rtree.h`

Replace the linear scan in `boost_cpu_exact_backend.cpp` for `contains` and `intersects` queries with an in-memory R-tree index (Boost.Geometry `rtree` with `rstar` packing algorithm). For collections with > 10 000 geometries the current O(n) scan becomes the dominant query cost.

**Implementation Notes:**
- Add `SpatialIndex` class in a new `spatial_index.cpp`; wrap `boost::geometry::index::rtree<value, bgi::rstar<16>>`.
- Index is built lazily on first spatial query and cached per collection handle; invalidated on write.
- Expose `SpatialIndex::bulkLoad(geometries)` for cold-start performance (STR packing is 3–5× faster than incremental insert for read-heavy workloads).
- Index memory usage must be reported via the existing structured audit log (`geo_index_bytes_allocated` field) so operators can observe RSS growth.

**Performance Targets:**
- `intersects` query over 1 M point geometries: ≤ 5 ms p99 with R-tree vs ~2 s with linear scan.
- Bulk-load of 1 M geometries into the R-tree ≤ 3 s wall clock.

**Scientific References:**
- [1] Guttman, A. (1984). R-Trees: A Dynamic Index Structure for Spatial Searching. *Proceedings of the 1984 ACM SIGMOD International Conference on Management of Data*, 47–57. https://doi.org/10.1145/602259.602266
- [2] Beckmann, N., Kriegel, H.-P., Schneider, R., & Seeger, B. (1990). The R*-Tree: An Efficient and Robust Access Method for Points and Rectangles. *Proceedings of the 1990 ACM SIGMOD International Conference on Management of Data*, 322–331. https://doi.org/10.1145/93597.98741
- [3] Leutenegger, S. T., Lopez, M. A., & Edgington, J. (1997). STR: A Simple and Efficient Algorithm for R-Tree Packing. *Proceedings of the 13th IEEE International Conference on Data Engineering*, 497–506. https://doi.org/10.1109/ICDE.1997.582015

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

### Temporal-Spatial Queries (Location at Time T)
**Priority:** Medium
**Target Version:** v2.4.0
**Status:** ✅ Implemented in `include/geo/temporal_spatial_query.h` + `src/geo/temporal_spatial_query.cpp`

Bridge between the temporal versioning system (`SystemVersionedTable`) and geospatial queries.
Answers questions of the form "where was entity X at time T?" or "which entities were inside
region R at time T?".

**What was implemented:**
- `TemporalSpatialQuery::extractGeometry(doc, field)` — parse a GeoJSON geometry from a
  named field of a `VersionedDocument`; field value may be a JSON string or an embedded JSON
  object.
- `TemporalSpatialQuery::locationAtTime(table, key, as_of)` — return the geometry stored in
  the version of `key` that was current at `as_of` (ms since epoch); returns `std::nullopt`
  when the key did not exist at that time or the geometry field is absent / invalid.
- `TemporalSpatialQuery::allLocationsAtTime(table, as_of)` — return `(key, geometry)` pairs
  for every entity alive at time T that carries a parseable geometry field.
- `TemporalSpatialQuery::entitiesInBBoxAtTime(table, bbox, as_of)` — filter alive entities
  at time T whose geometry centroid falls inside the given axis-aligned bounding box (WGS-84).
- `TemporalSpatialQuery::entitiesWithinDistanceAtTime(table, lon, lat, distance_m, as_of)` —
  filter alive entities at time T within `distance_m` metres of a centre point; uses the
  Haversine spherical-earth formula via `haversineDistanceM()` from `spatial_join.h`.
- `TemporalSpatialQuery::entitiesWithinDistanceAtTimeSorted(...)` — same as above but returns
  `(document, distance_m)` pairs sorted ascending by distance.

**Implementation Notes:**
- All methods are `static` and stateless; thread-safe as long as the `SystemVersionedTable`
  reference remains stable during the call.
- Geometry is always read from the named field in `VersionedDocument::data` (default:
  `"location"`); a custom field name can be supplied via the `geo_field` parameter.
- Invalid or missing geometry fields produce `std::nullopt` / skip that row (no exception
  thrown to the caller); parse failures are logged at WARN level via `THEMIS_WARN`.
- The centroid representative point is used for BBox and distance filters: Point geometries
  use their single coordinate directly; all other types use `GeometryInfo::computeCentroid()`.

**Performance Targets:**
- `locationAtTime` on a table with 100 K rows: ≤ 1 ms (delegates to `SystemVersionedTable::getAsOf` which is O(log n)).
- `entitiesWithinDistanceAtTime` over 10 K alive entities: ≤ 50 ms single-threaded (linear scan; R-tree optimisation deferred to a future release).

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

---

### Geo Point Clustering: DBSCAN and k-means
**Priority:** Medium
**Target Version:** v2.4.0
**Status:** ✅ Implemented in `include/geo/geo_clustering.h` + `src/geo/geo_clustering.cpp`

**What was implemented:**
- `dbscanCluster(points, DbscanConfig)` — density-based spatial clustering:
  - Haversine distance for all pairwise neighbour queries
  - Noise points receive label `kDbscanNoise` (-1)
  - Non-Point geometries are silently assigned noise label
  - O(n²) complexity; suitable for collections up to ~50 000 points
- `kmeansCluster(points, KMeansConfig)` — Lloyd's algorithm:
  - Deterministic initialisation (first k distinct points, `seed == 0`) or
    k-means++ probabilistic seeding (`seed != 0`, LCG PRNG)
  - Centroid updates via arithmetic mean of (lon, lat) — valid for clusters
    spanning < a few hundred kilometres
  - Convergence check: stops early when all centroid shifts ≤ `tolerance_m`
  - Non-Point geometries receive label -1 and are excluded from centroid
    computation
  - Throws `std::invalid_argument` when k == 0 or k > valid point count
- 20 unit tests in `tests/geo/test_geo_clustering.cpp`

**Performance Targets (design):**
- DBSCAN: 10 000 points at 500 m epsilon in ≤ 5 s single-threaded (CPU).
- k-means: k=10, 100 000 points, 100 iterations in ≤ 2 s single-threaded (CPU).

**Scientific References:**
- [1] Ester, M., Kriegel, H.-P., Sander, J., & Xu, X. (1996). A Density-Based Algorithm for Discovering Clusters in Large Spatial Databases with Noise. *Proceedings of the 2nd International Conference on Knowledge Discovery and Data Mining (KDD-96)*, 226–231. https://dl.acm.org/doi/10.5555/3001460.3001507
- [2] Lloyd, S. P. (1982). Least Squares Quantization in PCM. *IEEE Transactions on Information Theory*, 28(2), 129–137. https://doi.org/10.1109/TIT.1982.1056489
- [3] Arthur, D., & Vassilvitskii, S. (2007). k-means++: The Advantages of Careful Seeding. *Proceedings of the 18th Annual ACM-SIAM Symposium on Discrete Algorithms (SODA '07)*, 1027–1035. https://dl.acm.org/doi/10.5555/1283383.1283494

## Security / Reliability

- All GeoJSON inputs must be validated for coordinate range (longitude ∈ [−180, 180], latitude ∈ [−90, 90]) before being passed to any backend; out-of-range values are rejected with a typed error, not silently clamped.
- `GeometryCollection` recursive parsing is bounded by a configurable depth limit (default 8) to prevent stack-overflow attacks from deeply nested inputs.
- CUDA kernel failures always trigger the circuit-breaker and fall back to CPU; partial GPU results are never returned to the caller.
- The structured audit log entry for every GPU↔CPU backend switch must include timestamp, operation name, error code, and collection ID to support incident investigation.

---

## Planned: Spherical WGS-84 Ellipsoid Geometry
**Priority:** Medium
**Target Version:** v2.5.0
**Status:** Planned (Issue: #1744)

Replace the current Haversine/spherical-earth approximation with proper WGS-84 ellipsoid calculations for ST_Distance and ST_Buffer. This improves geodesic accuracy by up to 0.5% for long-distance queries near the poles or equator.

**Scope:**
- Affected files: `cpu_backend.cpp`, `boost_cpu_exact_backend.cpp`, `spatial_join.cpp`, `temporal_spatial_query.cpp`
- Add `GeoDistanceFormula::Ellipsoidal` variant alongside the existing `Haversine` and `Vincenty` entries in `include/geo/spatial_backend.h`
- Implement Vincenty's formulae as the ellipsoidal backend; Karney (2013) geodesics as the high-precision option

**Design Constraints:**
- Ellipsoidal distance must not regress performance by more than 5× vs Haversine for 1 M point pairs
- All GPU kernel inputs remain WGS-84 (lat/lon in degrees); ellipsoid parameters are passed as constants
- Existing ST_Distance behaviour is preserved by default; `geo.distance_formula = "ellipsoidal"` opt-in config key

**Performance Targets:**
- Ellipsoidal ST_Distance over 1 M point pairs: ≤ 500 ms single-threaded (CPU)
- GPU ellipsoidal kernel: ≤ 50 ms on A10G-class hardware for 1 M pairs

**Scientific References:**
- [1] Vincenty, T. (1975). Direct and Inverse Solutions of Geodesics on the Ellipsoid with Application of Nested Equations. *Survey Review*, 23(176), 88–93. https://doi.org/10.1179/sre.1975.23.176.88
- [2] Karney, C. F. F. (2013). Algorithms for Geodesics. *Journal of Geodesy*, 87(1), 43–55. https://doi.org/10.1007/s00190-012-0578-z
- [3] Bowring, B. R. (1983). The Geodesic Line on the Surface of the Ellipsoid. *Survey Review*, 27(210), 200–206. https://doi.org/10.1179/sre.1983.27.210.200

---

## Planned: Configurable Precision Mode
**Priority:** Medium
**Target Version:** v2.2.0
**Status:** In Progress (Issue: #1742)

Expose `GeoPrecisionMode` selection to AQL callers via a query hint or collection-level config so that operators can trade exact geometric accuracy for query throughput.

**Scope:**
- `getBackendForPrecision(GeoPrecisionMode)` factory already exists in `include/geo/spatial_backend.h`
- Add AQL query hint `/*+ GEO_PRECISION(approximate) */` processed by `src/query/`
- Add `geo.default_precision` config key (values: `"exact"` | `"approximate"`)

**Design Constraints:**
- `Approximate` mode must never produce false negatives (safe as a spatial pre-filter)
- `Exact` mode must not regress performance vs current baseline
- Mode selection is per-query; collection-level default is overridable per statement

**Scientific References:**
- [1] Böhm, C., Berchtold, S., & Keim, D. A. (2001). Searching in High-Dimensional Spaces: Index Structures for Improving the Performance of Multimedia Databases. *ACM Computing Surveys*, 33(3), 322–373. https://doi.org/10.1145/502807.502809
- [2] Rigaux, P., Scholl, M., & Voisard, A. (2001). *Spatial Databases: With Application to GIS*. Morgan Kaufmann. ISBN 978-1558605886.

---

## Planned: GPU-Accelerated Clustering
**Priority:** Low
**Target Version:** v2.3.0
**Status:** Planned

Accelerate DBSCAN and k-means geo-clustering on GPU to lift the O(n²) CPU barrier for large datasets (> 100 K points).

**Scope:**
- New CUDA kernel `geo_clustering_kernels.cu` with a pairwise Haversine distance kernel
- GPU-accelerated k-means: centroid update step in CUDA (dominant cost for large k)
- DBSCAN GPU port: leverages the existing CUDA device detection in `device_detector.h`

**Design Constraints:**
- GPU clustering results must match CPU reference within `tolerance_m` convergence threshold
- Falls back to CPU path via circuit-breaker on any CUDA error

**Performance Targets:**
- DBSCAN: 100 000 points at 500 m epsilon in ≤ 5 s GPU vs ~500 s CPU (>100× speedup)
- k-means: k=20, 1 M points, 100 iterations in ≤ 10 s GPU

**Scientific References:**
- [1] Andrade, G., Ramos, G., Madeira, D., Sachetto, R., Ferreira, R., & Rocha, L. (2013). G-DBSCAN: A GPU Accelerated Algorithm for Density-Based Clustering. *Procedia Computer Science*, 18, 369–378. https://doi.org/10.1016/j.procs.2013.05.200
- [2] Zhao, W., Ma, H., & He, Q. (2009). Parallel k-Means Clustering Based on MapReduce. *Proceedings of the 1st International Conference on Cloud Computing (CloudCom 2009)*, Lecture Notes in Computer Science, vol 5931, 674–679. https://doi.org/10.1007/978-3-642-10665-1_71
- [3] Li, Y., Zhao, K., Chu, X., & Liu, J. (2013). Speeding Up k-Means Algorithm by GPUs. *Journal of Computer and System Sciences*, 79(2), 216–229. https://doi.org/10.1016/j.jcss.2012.04.007

---

## Planned: CUDA Kernels for ST_BUFFER, ST_UNION, ST_DIFFERENCE
**Priority:** Medium
**Target Version:** v2.2.0
**Status:** Planned (ST_BUFFER/UNION/DIFFERENCE currently CPU-only on GPU path)

Implement dedicated CUDA kernels for the set-operation ST_ functions so that the GPU path no longer delegates to CPU for these operations.

**Scope:**
- New kernel `stBufferKernel` in `src/acceleration/cuda/geo_kernels.cu`
- New kernels `stUnionKernel`, `stDifferenceKernel` using parallel polygon clipping (Greiner-Hormann on GPU)
- Integration via `GpuKernelDispatcher` dispatch table

**Design Constraints:**
- Kernel output must pass the existing `tests/geo/test_geo_st_buffer.cpp` parametric tests on GPU path
- Maximum input polygon vertex count bounded by 4096 per geometry to fit in shared memory

**Performance Targets:**
- ST_BUFFER: 10 000 Point geometries at 500 m radius in ≤ 20 ms on A10G (10× speedup vs CPU baseline)
- ST_UNION: 1 000 polygon pairs in ≤ 10 ms on A10G

**Scientific References:**
- [1] Greiner, G., & Hormann, K. (1998). Efficient Clipping of Arbitrary Polygons. *ACM Transactions on Graphics*, 17(2), 71–83. https://doi.org/10.1145/274363.274364
- [2] Liu, F., Zhao, H., Hu, Z., & Shen, X. (2010). GPU-Accelerated Geo-Processing. *Proceedings of the 2010 18th International Conference on Geoinformatics*, 1–6. https://doi.org/10.1109/GEOINFORMATICS.2010.5567729
- [3] Owens, J. D., Houston, M., Luebke, D., Green, S., Stone, J. E., & Phillips, J. C. (2008). GPU Computing. *Proceedings of the IEEE*, 96(5), 879–899. https://doi.org/10.1109/JPROC.2008.917757

---

## References

[R1] Guttman, A. (1984). R-Trees: A Dynamic Index Structure for Spatial Searching. *Proceedings of the 1984 ACM SIGMOD International Conference on Management of Data*, 47–57. https://doi.org/10.1145/602259.602266

[R2] Beckmann, N., Kriegel, H.-P., Schneider, R., & Seeger, B. (1990). The R*-Tree: An Efficient and Robust Access Method for Points and Rectangles. *Proceedings of the 1990 ACM SIGMOD International Conference on Management of Data*, 322–331. https://doi.org/10.1145/93597.98741

[R3] Ester, M., Kriegel, H.-P., Sander, J., & Xu, X. (1996). A Density-Based Algorithm for Discovering Clusters in Large Spatial Databases with Noise. *Proceedings of the 2nd International Conference on Knowledge Discovery and Data Mining (KDD-96)*, 226–231. https://dl.acm.org/doi/10.5555/3001460.3001507

[R4] Lloyd, S. P. (1982). Least Squares Quantization in PCM. *IEEE Transactions on Information Theory*, 28(2), 129–137. https://doi.org/10.1109/TIT.1982.1056489

[R5] Vincenty, T. (1975). Direct and Inverse Solutions of Geodesics on the Ellipsoid with Application of Nested Equations. *Survey Review*, 23(176), 88–93. https://doi.org/10.1179/sre.1975.23.176.88

[R6] Karney, C. F. F. (2013). Algorithms for Geodesics. *Journal of Geodesy*, 87(1), 43–55. https://doi.org/10.1007/s00190-012-0578-z

[R7] Greiner, G., & Hormann, K. (1998). Efficient Clipping of Arbitrary Polygons. *ACM Transactions on Graphics*, 17(2), 71–83. https://doi.org/10.1145/274363.274364

[R8] Andrade, G., et al. (2013). G-DBSCAN: A GPU Accelerated Algorithm for Density-Based Clustering. *Procedia Computer Science*, 18, 369–378. https://doi.org/10.1016/j.procs.2013.05.200

[R9] Brinkhoff, T., Kriegel, H.-P., & Seeger, B. (1993). Efficient Processing of Spatial Joins Using R-Trees. *Proceedings of the 1993 ACM SIGMOD International Conference on Management of Data*, 237–246. https://doi.org/10.1145/170035.170075

[R10] Open Geospatial Consortium. (2010). OpenGIS® Implementation Standard for Geographic Information – Simple Feature Access – Part 1: Common Architecture (Version 1.2.1). OGC 06-103r4. https://www.ogc.org/standards/sfa
