<!-- Status: current | validated: 2026-04-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/geo/ROADMAP.md -->

# Roadmap — Geo Module (Public Headers)

> Implementation roadmap: `../../src/geo/ROADMAP.md`

## Current Status

v2.3.0 — Production-ready. 16 public headers covering spatial indexing, GPU dispatch, clustering, raster, temporal-spatial queries, and the new abstract interface layer.

## Completed ✅

- [x] R-tree spatial index (`geo_rtree.h`)
- [x] Pluggable spatial backend abstraction (`spatial_backend.h`)
- [x] Streaming spatial join (`spatial_join.h`)
- [x] GPU kernel dispatcher (`gpu_kernel_dispatcher.h`)
- [x] Device auto-detection (`device_detector.h`)
- [x] Raster grid and heatmap (`raster.h`)
- [x] Temporal-spatial queries (`temporal_spatial_query.h`)
- [x] `IGeoIndex` abstract spatial index interface (`geo_index.h`)
- [x] `IGeoJSONGeometry` + concrete GeoJSON types (`geojson_geometry.h`)
- [x] `ISpatialJoinFilter` composable predicate interface (`spatial_join_filter.h`)
- [x] `IRTreeCursor` pull-based cursor API (`rtree_cursor.h`)
- [x] `IRasterQueryInterface` raster query interface + null stub (`raster_query_interface.h`)
- [x] `ITemporalSpatialQueryBuilder` fluent builder API (`temporal_spatial_query_builder.h`)

## Planned

- [ ] Spherical geometry (WGS-84 ellipsoid) in `spatial_backend.h` — Issue #1744 (Target: v2.4.0)
- [ ] GPU-accelerated DBSCAN / k-means in `geo_clustering.h` (Target: v2.4.0)
- [ ] CUDA kernels for `ST_BUFFER`, `ST_UNION`, `ST_DIFFERENCE` in `gpu_kernel_dispatcher.h` (Target: v2.3.0)

## Implementation Phases

### Phase 1: Core Spatial (Complete ✅)
- [x] R-tree index, spatial backend, spatial join, clustering

### Phase 2: GPU Acceleration (Complete ✅)
- [x] GPU kernel dispatcher, device detector

### Phase 3: Abstract Interface Layer (Complete ✅)
- [x] IGeoIndex, IGeoJSONGeometry, ISpatialJoinFilter, IRTreeCursor, IRasterQueryInterface, ITemporalSpatialQueryBuilder

### Phase 4: Extended Operations (In Progress 🚧)
- [ ] Spherical geometry, GPU clustering, GPU overlay operations

## Production Readiness Checklist

- [x] 16 public headers compile cleanly
- [x] 20 CI test targets pass (+ GeoFutureInterfacesFocusedTests)
- [x] GPU/CPU backend parity tested
- [x] IGeoJSONGeometry RFC 7946 compliance (validate() on all 5 geometry types)
- [x] ISpatialJoinFilter truth-table coverage (AND/OR/NOT + 5 built-in predicates)
- [x] ITemporalSpatialQueryBuilder all three window types validated
- [ ] Spherical geometry WGS-84 support
- [ ] GPU DBSCAN/k-means
