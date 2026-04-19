<!-- Status: current | validated: 2026-04-13 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/geo/ROADMAP.md -->

# Roadmap — Geo Module (Public Headers)

> Implementation roadmap: `../../src/geo/ROADMAP.md`

## Current Status

v2.3.0 — Production-ready. 11 public headers covering spatial indexing, GPU dispatch, clustering, raster, temporal-spatial queries, and FAISS GPU k-NN bridge.

## Completed ✅

- [x] R-tree spatial index (`geo_rtree.h`)
- [x] Pluggable spatial backend abstraction (`spatial_backend.h`)
- [x] Streaming spatial join (`spatial_join.h`)
- [x] GPU kernel dispatcher (`gpu_kernel_dispatcher.h`)
- [x] Device auto-detection (`device_detector.h`)
- [x] Raster grid and heatmap (`raster.h`)
- [x] Temporal-spatial queries (`temporal_spatial_query.h`)
- [x] GPU-accelerated DBSCAN via CUDA Haversine adjacency kernel — `GpuClusteringConfig` added to `geo_clustering.h`
- [x] CUDA batch ST_BUFFER kernel for Point geometries; ST_UNION/ST_DIFFERENCE delegate to CPU exact path
- [x] FAISS GPU k-NN bridge (`geo_faiss_knn.h`) — ECEF projection + FAISS GPU FLAT_L2 for spatial k-NN and radius search
- [x] Fixed `CpuParallelBackend::batchIntersects` correctness bug (always returned 0)
- [x] `stBuffer` / `stUnion` / `stDifference` / `geodesicDistance` wired in all GPU backends

## Planned

- [x] Spherical geometry (WGS-84 ellipsoid) in `spatial_backend.h` — Issue #1744 (v2.5.0, 2026-04-19)
  - CRS-aware `GeoPoint`/`GeoLineString`/`GeoPolygon`/`GeoMultiPolygon`/`GeoGeometryCollection` in `geo_json_geometry.h`
  - `CrsId` enum (WGS84 / EPSG3857 / EPSG4978 / Custom); CRS required at construction — no implicit WGS-84
  - `ValidationResult` with `validate()` on all geometry types; right-hand-rule winding enforcement on `GeoPolygon`
  - `ISpatialJoinFilter` + built-in predicates (Intersects/Contains/Within/Touches/DWithin) + And/Or/Not composables in `spatial_join_filter.h`
  - 12 tests GJS-01..12 (test_geo_json_geometry_focused), 10 tests SJF-01..10 (test_spatial_join_filter_focused)

## Implementation Phases

### Phase 1: Core Spatial (Complete ✅)
- [x] R-tree index, spatial backend, spatial join, clustering

### Phase 2: GPU Acceleration (Complete ✅)
- [x] GPU kernel dispatcher, device detector

### Phase 3: Extended Operations (Complete ✅)
- [x] GPU DBSCAN adjacency kernel, GPU ST_BUFFER, FAISS k-NN bridge
- [x] Spherical geometry WGS-84 (v2.5.0, 2026-04-19 — geo_json_geometry.h + spatial_join_filter.h)
## Production Readiness Checklist

- [x] 11 public headers compile cleanly
- [x] 19 CI test targets pass
- [x] GPU/CPU backend parity tested
- [x] GPU DBSCAN/k-means (DBSCAN GPU adjacency + k-Means FAISS GPU path)
- [x] GPU ST_BUFFER Point kernel; ST_UNION/ST_DIFFERENCE CPU delegation
- [x] Spherical geometry WGS-84 support (v2.5.0, 2026-04-19)
