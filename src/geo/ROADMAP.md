# Geo Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — CPU-based geospatial queries are well-tested. GPU-accelerated backend is implemented with CPU fallback via circuit breaker. S2/H3 cell indexing is supported. Full GeoJSON import/export and ST_BUFFER are still in progress.

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
- [ ] Full GeoJSON parsing (all geometry types) (Target: Q2 2026)
- [ ] ST_BUFFER operation implementation (Target: Q2 2026)
- [ ] CUDA kernel dispatch for GPU backend (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Complete GeoJSON spec coverage (GeometryCollection, MultiPolygon)
- [ ] ST_BUFFER: expand geometry by a fixed distance
- [ ] ST_UNION and ST_DIFFERENCE geometry operations
- [ ] Spatial JOIN support (find all pairs within distance)
- [ ] R-tree index integration for CPU backend
- [ ] Configurable precision mode (exact vs. approximate)

### Long-term (6-12 months)
- [ ] ROCm/HIP GPU backend for AMD hardware
- [ ] Spherical geometry support (WGS-84 ellipsoid)
- [ ] Raster data query support (elevation, heatmaps)
- [ ] Temporal-spatial queries (location at time T)
- [ ] Clustering algorithms: DBSCAN, k-means for geo points
- [ ] Tile server integration for map visualization

## Implementation Phases

### Phase 1: CPU Geospatial Backend (Status: Completed)
- [x] Implemented CPU-based geospatial backend using Boost.Geometry for exact calculations
- [x] Implemented `contains`, `intersects`, and `distance` geometry operations
- [x] Integrated S2 cell indexing for hierarchical spatial lookups
- [x] Integrated H3 hexagonal grid indexing for uniform spatial binning
- [x] Added structured audit log for backend selection events (GPU vs CPU)

### Phase 2: GPU Backend Stub and Device Detection (Status: In Progress)
- [~] Implemented GPU backend stub with automatic CPU fallback (`geo/gpu_backend_stub.cpp`)
- [~] Implemented circuit-breaker fallback when no CUDA-capable device is present
- [~] Implement runtime GPU device discovery and capability reporting (`geo/device_detector.cpp`)

### Phase 3: Full GeoJSON, Spatial Index, and CUDA Dispatch (Status: Planned)
- [ ] Implement full GeoJSON RFC 7946 parsing for all geometry types including `GeometryCollection` and `MultiPolygon`
- [ ] Implement R-tree spatial index for sub-linear CPU query performance
- [ ] Implement `ST_BUFFER` operation expanding geometry by a fixed distance
- [ ] Implement CUDA kernel dispatch for distance and containment on GPU (`cuda/geo_kernels.cu`)
- [ ] Implement spatial JOIN finding all point pairs within a configurable distance threshold

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [x] Integration tests (CPU backend, GPU fallback, S2/H3 indexing)
- [ ] Performance benchmarks (CPU vs GPU throughput)
- [x] Security audit (no code execution from geometry inputs)
- [x] Documentation complete (GPU runbook, roadmap, future enhancements)
- [x] API stability guaranteed for spatial query API

## Known Issues & Limitations
- Full GeoJSON parsing is incomplete; some geometry types may not parse correctly
- ST_BUFFER operation is planned but not yet implemented
- CUDA kernels for GPU dispatch are not yet written; GPU backend uses CPU fallback
- ROCm/HIP support is not available
- Raster data is not supported

## Breaking Changes
- GeoJSON parsing will become stricter when full spec compliance is added (may reject previously accepted malformed inputs)
