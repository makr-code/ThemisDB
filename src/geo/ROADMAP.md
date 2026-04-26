> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Geo Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Production-Ready** — CPU-based geospatial queries are well-tested. GPU-accelerated backend is implemented with CPU fallback via circuit breaker. S2/H3 cell indexing is supported. Full GeoJSON RFC 7946 import/export is complete. ST_BUFFER is complete on CPU and Boost backends; the CUDA backend dispatches a GPU batch-buffer kernel for Point geometries and delegates polygon ST_BUFFER to the CPU exact path. CUDA kernels for distance and containment are implemented in `gpu_backend_cuda.cu`; GPU ST_UNION and ST_DIFFERENCE delegate to the CPU Greiner–Hormann implementation. Raster data queries (elevation sampling, bbox extraction, Gaussian KDE heatmaps) are implemented in `include/geo/raster.h` + `src/geo/raster.cpp`. Temporal-spatial queries (location at time T) are implemented in `include/geo/temporal_spatial_query.h` + `src/geo/temporal_spatial_query.cpp`. FAISS GPU k-NN bridge (`include/geo/geo_faiss_knn.h` + `src/geo/geo_faiss_knn.cpp`) provides GPU-accelerated spatial k-NN and radius search via ECEF projection. GPU-accelerated DBSCAN uses a CUDA Haversine adjacency kernel for datasets ≤ 32768 points.

## Completed ✅
- [x] CPU-based geospatial backend (exact calculations)
- [x] Boost.Geometry exact CPU backend
- [x] GPU-accelerated geospatial backend with automatic CPU fallback
- [x] Circuit-breaker based fallback when no GPU device is present
- [x] 2D and 3D spatial query support
- [x] Geometry operations: contains, intersects, distance
- [x] ST_UNION and ST_DIFFERENCE geometry operations (CPU-exact, Boost, GPU-fallback backends; AQL `ST_UNION` / `ST_DIFFERENCE` functions)
- [x] S2 cell indexing support
- [x] H3 hexagonal grid indexing support
- [x] GPU backend device discovery and runbook documentation
- [x] Structured audit log for GPU/CPU backend switches
- [x] Full GeoJSON RFC 7946 spec coverage (all 7 geometry types including `GeometryCollection` and `MultiPolygon`)
- [x] ST_BUFFER operation (CPU-exact and Boost backends; GPU backend delegates to CPU with audit log)
- [x] CUDA kernel dispatch for distance and containment (`src/acceleration/cuda/geo_kernels.cu` + `src/geo/gpu_backend_cuda.cu`)
- [x] Raster data queries (elevation sampling, bbox extraction, Gaussian KDE heatmaps) (`include/geo/raster.h` + `src/geo/raster.cpp`)
- [x] Temporal-spatial queries (location at time T) (`include/geo/temporal_spatial_query.h` + `src/geo/temporal_spatial_query.cpp`)
- [x] Complete GeoJSON spec coverage (GeometryCollection, MultiPolygon) (Issue: #1737)
- [x] ST_BUFFER: expand geometry by a fixed distance (Issue: #1738)
- [x] ST_UNION and ST_DIFFERENCE geometry operations (Issue: #1739)
- [x] Spatial JOIN support (find all pairs within distance) (Issue: #1740)
- [x] R-tree index integration for CPU backend (Issue: #1741)
- [x] ROCm/HIP GPU backend for AMD hardware (Issue: #1743)
- [x] Raster data query support (elevation, heatmaps) (Issue: #1745)
- [x] Temporal-spatial queries (location at time T) (Issue: #1746)
- [x] Clustering algorithms: DBSCAN, k-means for geo points (Issue: #1747)
- [x] Tile server integration for map visualization (Issue: #1748)
- [x] Configurable precision mode (exact vs. approximate) (Issue: #1742)
- [x] Pull-based R-tree cursor API: `IRTreeCursor`, `IGeoIndex`, `GeoRTreeIndex` (`include/geo/rtree_cursor.h`, `src/geo/rtree_cursor.cpp`) — `openRangeCursor(MBR)`, `openKNNCursor(Coordinate, k)`, `estimatedResultCount()`; `CursorStatus::STALE` on index mutation (Target: v2.5.0)
- [x] Fluent temporal-spatial query builder: `ITemporalSpatialQueryBuilder`, `TemporalSpatialQueryBuilder`, `BuiltTemporalSpatialQuery`, `TimeWindowType` (`POINT_IN_TIME`, `INTERVAL`, `SLIDING_WINDOW`) (`include/geo/temporal_spatial_query_builder.h`, `src/geo/temporal_spatial_query_builder.cpp`) (Target: v2.5.0)
- [x] Typed raster query interface: `IRasterQueryInterface`, `RasterGridQueryImpl`, `NoOpRasterQueryImpl`, `RasterConfig`, `RasterStatus`, `RasterResult` (`include/geo/raster_query_interface.h`, `src/geo/raster_query_interface.cpp`); guarded by `THEMIS_ENABLE_RASTER` (Target: v2.5.0)
- [x] GeoJSON geometry class hierarchy: `IGeoJSONGeometry`, `GeoPoint`, `GeoLineString`, `GeoPolygon`, `GeoMultiPolygon`, `GeoGeometryCollection`; `BBox`, `ValidationResult`, `CrsId` (`include/geo/geo_json_geometry.h`, `src/geo/geo_json_geometry.cpp`) (Target: v2.5.0)
- [x] Composable spatial join filters: `ISpatialJoinFilter`, `IntersectsFilter`, `ContainsFilter`, `WithinFilter`, `TouchesFilter`, `DWithinFilter`, `AndFilter`, `OrFilter`, `NotFilter`; `SpatialJoinFilter` factory namespace (`include/geo/spatial_join_filter.h`, `src/geo/spatial_join_filter.cpp`) (Target: v2.5.0)

## In Progress 🚧
<!-- No items currently in progress -->

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Configurable precision mode (exact vs. approximate) (Issue: #1742)

### Long-term (6-12 months)
- [I] Spherical geometry support (WGS-84 ellipsoid) (Issue: #1744)

## Implementation Phases

### Phase 1: CPU Geospatial Backend (Status: Completed)
- [x] Implemented CPU-based geospatial backend using Boost.Geometry for exact calculations
- [x] Implemented `contains`, `intersects`, and `distance` geometry operations
- [x] Integrated S2 cell indexing for hierarchical spatial lookups
- [x] Integrated H3 hexagonal grid indexing for uniform spatial binning
- [x] Added structured audit log for backend selection events (GPU vs CPU)

### Phase 2: GPU Backend Stub and Device Detection (Status: Completed)
- [x] Implemented GPU backend stub with automatic CPU fallback (`geo/gpu_backend_stub.cpp`) (Issue: #1756)
- [x] Implemented circuit-breaker fallback when no CUDA-capable device is present (Issue: #1757)
- [x] Implement runtime GPU device discovery and capability reporting (`include/geo/device_detector.h` + `src/geo/device_detector.cpp`; wraps `themis::gpu::DeviceDiscovery` with geo-specific compute-capability and VRAM checks) (Issue: #1758)

### Phase 3: Full GeoJSON, Spatial Index, and CUDA Dispatch (Status: Completed)
- [x] Implement full GeoJSON RFC 7946 parsing for all geometry types including `GeometryCollection` and `MultiPolygon` (Issue: #1749)
- [x] Implement R-tree spatial index for sub-linear CPU query performance (Issue: #1750)
- [x] Implement `ST_BUFFER` operation expanding geometry by a fixed distance (Issue: #1751)
- [x] Implement CUDA kernel dispatch for distance and containment on GPU (`cuda/geo_kernels.cu`) (Issue: #1752)
- [x] Implement spatial JOIN finding all point pairs within a configurable distance threshold (Issue: #1753)

### Phase 4: Advanced Features (Status: Completed)
- [x] Implement DBSCAN and k-means geo-point clustering (`geo_clustering.cpp`, `geo_clustering.h`) (Issue: #1747)
- [x] Implement raster data queries: elevation sampling, bbox extraction, Gaussian KDE heatmaps (`raster.cpp`, `raster.h`) (Issue: #1745)
- [x] Implement temporal-spatial queries (location at time T, entities within distance at time T) (`temporal_spatial_query.cpp`) (Issue: #1746)
- [x] Implement ROCm/HIP GPU backend for AMD hardware (`gpu_backend_hip.cpp`) (Issue: #1743)
- [x] Implement tile server integration (`tile_server.cpp`, `tile_server.h`) (Issue: #1748)
- [x] Implement runtime GPU device discovery and capability reporting (`device_detector.cpp`, `device_detector.h`) (Issue: #1758)

### Phase 5: Performance & Hardening (Status: Completed)
- [x] Unit test coverage > 80% across geo module — 20 focused test targets added (Issue: #1754)
- [x] Configurable precision mode: expose `GeoPrecisionMode` enum and `getBackendForPrecision()` factory to AQL callers (Issue: #1742)
- [x] GPU-accelerated DBSCAN via CUDA Haversine adjacency kernel (n ≤ 32768; larger datasets fall back to CPU) — `GpuClusteringConfig` controls the threshold
- [x] GPU batch ST_BUFFER kernel for Point geometries in `CudaBackend`; ST_UNION / ST_DIFFERENCE delegate to CPU Greiner–Hormann via `cpu_exact_`
- [x] FAISS GPU k-NN bridge: `GeoFaissKnn` (`include/geo/geo_faiss_knn.h`, `src/geo/geo_faiss_knn.cpp`) — ECEF 3D projection + FAISS GPU FLAT_L2 for spatial k-NN and radius search
- [x] Fixed correctness bug: `CpuParallelBackend::batchIntersects` now performs actual geometry checks (previously always returned 0, breaking Phase-2 verification in CUDA/OpenCL backends)
- [x] Added `stBuffer` / `stUnion` / `stDifference` / `geodesicDistance` overrides to all GPU backends (`CudaBackend`, `OpenCLBackend`, `ProductionGpuBackend`, `CpuParallelBackend`) — previously returned empty `GeometryInfo{}`
- [I] Spherical WGS-84 ellipsoid geometry support (Issue: #1744, Target: v2.5.0)

### Phase 6: Documentation & Acceptance (Status: Completed)
- [x] GPU backend runbook (`docs/gpu_runbooks.md#6`)
- [x] ROADMAP.md, ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md complete
- [x] API stability guaranteed for spatial query API
- [x] Security audit (no code execution from geometry inputs)
- [x] English documentation in `docs/en/geo/` (Issue: #1749, v2.2.0)

### Phase 7: v2.5.0 — Cursor API, Builders, Typed Interfaces (Status: Completed ✅)
- [x] Pull-based R-tree cursor API (`include/geo/rtree_cursor.h`, `src/geo/rtree_cursor.cpp`):
  - `CursorStatus` enum (`OK`, `END`, `STALE`)
  - `GeoIndexEntry` value type (key, geom, distance_m)
  - `IRTreeCursor` abstract interface: `next(GeoIndexEntry&)`, `estimatedResultCount()`
  - `IGeoIndex` abstract interface: `openRangeCursor(MBR)`, `openKNNCursor(Coordinate, k)`, `insert()`, `bulkLoad()`, `clear()`, `size()`
  - `GeoRTreeIndex` concrete implementation wrapping `GeoRTree`; version counter for stale-cursor detection
- [x] Fluent temporal-spatial query builder (`include/geo/temporal_spatial_query_builder.h`, `src/geo/temporal_spatial_query_builder.cpp`):
  - `TimeWindowType` enum: `POINT_IN_TIME`, `INTERVAL`, `SLIDING_WINDOW`
  - `BuiltTemporalSpatialQuery` immutable value type with `execute(SystemVersionedTable)` method
  - `ITemporalSpatialQueryBuilder` abstract fluent interface: `withinBBox()`, `withPredicate()`, `duringInterval()`, `atTime()`, `slidingWindow()`, `withGeoField()`, `build()`
  - `TemporalSpatialQueryBuilder` concrete implementation; `reset()` for reuse; throws `std::logic_error` when constraints are missing
- [x] Typed raster query interface (`include/geo/raster_query_interface.h`, `src/geo/raster_query_interface.cpp`):
  - `RasterConfig` with `maxTileSizeBytes()` (default 64 MiB)
  - `RasterStatus` enum: `OK`, `NOT_SUPPORTED`, `TILE_TOO_LARGE`, `INVALID_KEY`, `BACKEND_ERROR`, `INVALID_BBOX`
  - `RasterResult` value type with `grid`, `crs_wkt`, `error_message`
  - `IRasterQueryInterface` abstract interface
  - `NoOpRasterQueryImpl` returns `NOT_SUPPORTED` when `THEMIS_ENABLE_RASTER` is not defined
  - `RasterGridQueryImpl` full implementation guarded by `THEMIS_ENABLE_RASTER`
- [x] GeoJSON geometry OOP class hierarchy (`include/geo/geo_json_geometry.h`, `src/geo/geo_json_geometry.cpp`):
  - `CrsId` enum: `WGS84`, `WEB_MERCATOR`, `CUSTOM`
  - `BBox` struct; `ValidationError` struct; `ValidationResult` class
  - `IGeoJSONGeometry` abstract base: `type()`, `bbox()`, `validate()`, `toGeoJSON()`
  - `GeoPoint`, `GeoLineString`, `GeoPolygon` (right-hand-rule), `GeoMultiPolygon`, `GeoGeometryCollection`
- [x] Composable spatial join filters (`include/geo/spatial_join_filter.h`, `src/geo/spatial_join_filter.cpp`):
  - `ISpatialJoinFilter` abstract interface: `matches(GeometryInfo, GeometryInfo)`
  - `IntersectsFilter`, `ContainsFilter`, `WithinFilter`, `TouchesFilter`, `DWithinFilter` (Haversine)
  - `AndFilter`, `OrFilter`, `NotFilter` logical combinators
  - `SpatialJoinFilter` factory namespace: `intersects()`, `contains()`, `within()`, `touches()`, `dwithin()`, `and_()`, `or_()`, `not_()`
- [x] Tests: `tests/geo/test_raster_query_interface.cpp`, `tests/geo/test_temporal_spatial_query_builder.cpp`, `tests/geo/test_spatial_join_filter.cpp`

## Production Readiness Checklist
- [x] Unit tests coverage > 80% — 20 focused test targets in `tests/geo/` (Issue: #1754)
- [x] Integration tests (CPU backend, GPU fallback, S2/H3 indexing)
- [x] Performance benchmarks (CPU vs GPU throughput) (Issue: #1755) (PR: #3049, v1.5.0)
- [x] Security audit (no code execution from geometry inputs)
- [x] Documentation complete (GPU runbook, roadmap, future enhancements)
- [x] API stability guaranteed for spatial query API

## Known Issues & Limitations
- ST_UNION and ST_DIFFERENCE in GPU backends delegate to the CPU Greiner–Hormann implementation; dedicated CUDA kernels for polygon boolean set operations are deferred to v2.2.0
- ROCm/HIP geo kernel dispatch is implemented (`THEMIS_GEO_HIP`); requires `THEMIS_ENABLE_HIP=ON` and ROCm runtime
- GPU DBSCAN is limited to n ≤ `GpuClusteringConfig::gpu_dbscan_max_n` (default 32768) due to n² VRAM requirement; larger datasets fall back to CPU
- GPU k-Means ECEF assignment step via FAISS GPU is available through `GeoFaissKnn`; the inline L2 kernel dispatch in `geo_clustering.cpp` is wired but uses CPU fallback until the ANNKernelDispatch path is fully integrated (v1.4.0)

## Breaking Changes
- GeoJSON parsing is now strict: unknown geometry types and out-of-range WGS84 coordinates
  (longitude outside [-180, 180], latitude outside [-90, 90]) throw `std::runtime_error`.
  Compile with `-DTHEMIS_GEO_COMPAT_LAX=1` to skip coordinate range validation during a
  one-release migration window.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🟡 UNGENUTZT (kein Test, kein externer Aufrufer)

- `GeoFaissKnn` – KNN-Suche auf FAISS-Geo-Index; noch nicht extern verdrahtet
- `knnSearch` – Führt den eigentlichen kNN-Search-Call auf dem Geo-FAISS-Index durch
  > **Aktion:** Für jedes Symbol entscheiden: (1) Verdrahten, (2) Testen oder (3) als CANDIDATE_FOR_REMOVAL einplanen.

