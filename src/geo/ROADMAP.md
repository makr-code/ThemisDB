# Geo Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — CPU-based geospatial queries are well-tested. GPU-accelerated backend is implemented with CPU fallback via circuit breaker. S2/H3 cell indexing is supported. Full GeoJSON RFC 7946 import/export is complete. ST_BUFFER is complete on CPU and Boost backends. CUDA kernels for distance and containment are implemented in `gpu_backend_cuda.cu`; ST_BUFFER/UNION/DIFFERENCE CUDA kernels are deferred to v2.2.0. ST_UNION and ST_DIFFERENCE geometry operations are implemented in the CPU-exact, Boost, and GPU-fallback backends and exposed as `ST_UNION` / `ST_DIFFERENCE` AQL functions. Raster data queries (elevation sampling, bbox extraction, Gaussian KDE heatmaps) are implemented in `include/geo/raster.h` + `src/geo/raster.cpp`. DBSCAN and k-means clustering for geo points are implemented in `include/geo/geo_clustering.h` + `src/geo/geo_clustering.cpp`.

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
- [x] Clustering algorithms: DBSCAN and k-means for geo points (`include/geo/geo_clustering.h` + `src/geo/geo_clustering.cpp`)

## In Progress 🚧
<!-- No items currently in progress -->

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Complete GeoJSON spec coverage (GeometryCollection, MultiPolygon) (Issue: #1737)
- [x] ST_BUFFER: expand geometry by a fixed distance (Issue: #1738)
- [x] ST_UNION and ST_DIFFERENCE geometry operations (Issue: #1739)
- [x] Spatial JOIN support (find all pairs within distance) (Issue: #1740)
- [x] R-tree index integration for CPU backend (Issue: #1741)
- [I] Configurable precision mode (exact vs. approximate) (Issue: #1742)

### Long-term (6-12 months)
- [x] ROCm/HIP GPU backend for AMD hardware (Issue: #1743)
- [I] Spherical geometry support (WGS-84 ellipsoid) (Issue: #1744)
- [x] Raster data query support (elevation, heatmaps) (Issue: #1745)
- [I] Temporal-spatial queries (location at time T) (Issue: #1746)
- [x] Clustering algorithms: DBSCAN, k-means for geo points (Issue: #1747)
- [I] Tile server integration for map visualization (Issue: #1748)

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
- [x] Implement runtime GPU device discovery and capability reporting (`include/themis/gpu/device_discovery.h` + `src/gpu/device_discovery.cpp`) (Issue: #1758)

### Phase 3: Full GeoJSON, Spatial Index, and CUDA Dispatch (Status: Completed)
- [x] Implement full GeoJSON RFC 7946 parsing for all geometry types including `GeometryCollection` and `MultiPolygon` (Issue: #1749)
- [x] Implement R-tree spatial index for sub-linear CPU query performance (Issue: #1750)
- [x] Implement `ST_BUFFER` operation expanding geometry by a fixed distance (Issue: #1751)
- [x] Implement CUDA kernel dispatch for distance and containment on GPU (`cuda/geo_kernels.cu`) (Issue: #1752)
- [x] Implement spatial JOIN finding all point pairs within a configurable distance threshold (Issue: #1753)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1754)
- [x] Integration tests (CPU backend, GPU fallback, S2/H3 indexing)
- [I] Performance benchmarks (CPU vs GPU throughput) (Issue: #1755)
- [x] Security audit (no code execution from geometry inputs)
- [x] Documentation complete (GPU runbook, roadmap, future enhancements)
- [x] API stability guaranteed for spatial query API

## Known Issues & Limitations
- ST_BUFFER, ST_UNION, and ST_DIFFERENCE use CPU fallback for the GPU path; dedicated CUDA kernels for these set operations are deferred to v2.2.0
- ROCm/HIP support is not available
- CUDA kernels for GPU dispatch are not yet written; GPU backend uses CPU fallback
- ST_BUFFER, ST_UNION, and ST_DIFFERENCE use CPU fallback for the GPU backend; CUDA kernel dispatch is deferred to v2.1.0
- ROCm/HIP geo kernel dispatch is implemented (`THEMIS_GEO_HIP`); requires `THEMIS_ENABLE_HIP=ON` and ROCm runtime
- Raster data is not supported

## Breaking Changes
- GeoJSON parsing is now strict: unknown geometry types and out-of-range WGS84 coordinates
  (longitude outside [-180, 180], latitude outside [-90, 90]) throw `std::runtime_error`.
  Compile with `-DTHEMIS_GEO_COMPAT_LAX=1` to skip coordinate range validation during a
  one-release migration window.
