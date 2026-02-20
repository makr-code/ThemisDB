# Geo Module Roadmap

## Current Status
Production-ready for CPU-based geospatial queries. GPU-accelerated backend is implemented with CPU fallback via circuit breaker. S2/H3 cell indexing is supported.

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
