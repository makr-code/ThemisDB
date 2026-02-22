# Geo Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — CPU-based geospatial queries are well-tested. GPU-accelerated backend is implemented with CPU fallback via circuit breaker. S2/H3 cell indexing is supported. Full GeoJSON RFC 7946 import/export is complete. ST_BUFFER CPU implementation is complete; GPU CUDA kernel dispatch is deferred to v2.1.0.

## Completed ✅
- [x] CPU-based geospatial backend (exact calculations)
- [x] Boost.Geometry exact CPU backend
- [x] GPU-accelerated geospatial backend with automatic CPU fallback
- [x] Circuit-breaker based fallback when no GPU device is present
- [x] 2D and 3D spatial query support
- [x] Geometry operations: contains, intersects, distance
- [x] S2 cell indexing support
- [x] H3 hexagonal grid indexing support
- [x] GPU backend device discovery and runbook documentation
- [x] Structured audit log for GPU/CPU backend switches

## In Progress 🚧
- [P] Full GeoJSON parsing (all geometry types) (Target: Q2 2026) (Issue: #1734)
- [P] ST_BUFFER operation implementation (Target: Q2 2026) (Issue: #1735)
- [I] CUDA kernel dispatch for GPU backend (Target: Q3 2026) (Issue: #1736)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [P] Complete GeoJSON spec coverage (GeometryCollection, MultiPolygon) (Issue: #1737)
- [P] ST_BUFFER: expand geometry by a fixed distance (Issue: #1738)
- [I] ST_UNION and ST_DIFFERENCE geometry operations (Issue: #1739)
- [I] Spatial JOIN support (find all pairs within distance) (Issue: #1740)
- [P] R-tree index integration for CPU backend (Issue: #1741)
- [I] Configurable precision mode (exact vs. approximate) (Issue: #1742)

### Long-term (6-12 months)
- [I] ROCm/HIP GPU backend for AMD hardware (Issue: #1743)
- [I] Spherical geometry support (WGS-84 ellipsoid) (Issue: #1744)
- [I] Raster data query support (elevation, heatmaps) (Issue: #1745)
- [I] Temporal-spatial queries (location at time T) (Issue: #1746)
- [I] Clustering algorithms: DBSCAN, k-means for geo points (Issue: #1747)
- [I] Tile server integration for map visualization (Issue: #1748)

## Implementation Phases

### Phase 1: CPU Geospatial Backend (Status: Completed)
- [x] Implemented CPU-based geospatial backend using Boost.Geometry for exact calculations
- [x] Implemented `contains`, `intersects`, and `distance` geometry operations
- [x] Integrated S2 cell indexing for hierarchical spatial lookups
- [x] Integrated H3 hexagonal grid indexing for uniform spatial binning
- [x] Added structured audit log for backend selection events (GPU vs CPU)

### Phase 2: GPU Backend Stub and Device Detection (Status: In Progress)
- [I] Implemented GPU backend stub with automatic CPU fallback (`geo/gpu_backend_stub.cpp`) (Issue: #1756)
- [I] Implemented circuit-breaker fallback when no CUDA-capable device is present (Issue: #1757)
- [I] Implement runtime GPU device discovery and capability reporting (`geo/device_detector.cpp`) (Issue: #1758)

### Phase 3: Full GeoJSON, Spatial Index, and CUDA Dispatch (Status: Planned)
- [P] Implement full GeoJSON RFC 7946 parsing for all geometry types including `GeometryCollection` and `MultiPolygon` (Issue: #1749)
- [P] Implement R-tree spatial index for sub-linear CPU query performance (Issue: #1750)
- [P] Implement `ST_BUFFER` operation expanding geometry by a fixed distance (Issue: #1751)
- [!] Implement CUDA kernel dispatch for distance and containment on GPU (`cuda/geo_kernels.cu`) (Issue: #1752)
- [I] Implement spatial JOIN finding all point pairs within a configurable distance threshold (Issue: #1753)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1754)
- [x] Integration tests (CPU backend, GPU fallback, S2/H3 indexing)
- [I] Performance benchmarks (CPU vs GPU throughput) (Issue: #1755)
- [x] Security audit (no code execution from geometry inputs)
- [x] Documentation complete (GPU runbook, roadmap, future enhancements)
- [x] API stability guaranteed for spatial query API

## Known Issues & Limitations
- CUDA kernels for GPU dispatch are not yet written; GPU backend uses CPU fallback
- ST_BUFFER uses CPU fallback for the GPU backend; CUDA kernel dispatch is deferred to v2.1.0
- ROCm/HIP support is not available
- Raster data is not supported

## Breaking Changes
- GeoJSON parsing is now strict: unknown geometry types and out-of-range WGS84 coordinates
  (longitude outside [-180, 180], latitude outside [-90, 90]) throw `std::runtime_error`.
  Compile with `-DTHEMIS_GEO_COMPAT_LAX=1` to skip coordinate range validation during a
  one-release migration window.
