<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Geo Module (Public Headers)

All notable changes to the Geo module public headers are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).  
For implementation-level changes see `../../src/geo/CHANGELOG.md`.

## [Unreleased]
- (nothing pending)

## [2.5.0] — 2026-04-19
### Added
- `geo_json_geometry.h` + `src/geo/geo_json_geometry.cpp`: CRS-aware GeoJSON geometry
  type API — `IGeoJSONGeometry` abstract base, `CrsId` enum (WGS84/EPSG3857/EPSG4978/Custom),
  `ValidationResult`, `GeoPoint`, `GeoLineString`, `GeoPolygon` (right-hand-rule winding
  enforced), `GeoMultiPolygon`, `GeoGeometryCollection` — resolves Issue #1744
- `spatial_join_filter.h` + `src/geo/spatial_join_filter.cpp`: `ISpatialJoinFilter` abstract
  base + built-in predicates `IntersectsFilter`, `ContainsFilter`, `WithinFilter`,
  `TouchesFilter`, `DWithinFilter` (Haversine) + logical composables `AndFilter`, `OrFilter`,
  `NotFilter` + `SpatialJoinFilter::` factory namespace
- 12 unit tests GJS-01..12 in `tests/geo/test_geo_json_geometry.cpp`
  (CMake target: `test_geo_json_geometry_focused`)
- 10 unit tests SJF-01..10 in `tests/geo/test_spatial_join_filter.cpp`
  (CMake target: `test_spatial_join_filter_focused`)

## [2.2.0] — 2026-03-21
### Added
- Comprehensive CI workflow `geo-module-ci.yml` with 19 focused test targets
- English documentation `docs/en/geo/README.md`

## [1.7.0] — 2026-03-09
### Added
- `temporal_spatial_query.h`: `TemporalSpatialQuery` for time-window + spatial predicates
- `raster.h`: `RasterGrid`, `RasterSampleResult`, `HeatmapConfig`
- `tile_server.h`: `TileCoord` map tile utilities
- `gpu_kernel_dispatcher.h`: `GpuKernelDispatcher` CUDA kernel dispatch
- `device_detector.h`: `GeoDeviceDetector`, `GeoDeviceCapability`

## [1.0.0] — 2024-01-01
### Added
- `geo_rtree.h`: R-tree spatial index
- `spatial_backend.h`: `ISpatialComputeBackend` abstraction
- `spatial_join.h`: streaming spatial join
- `geo_ops_ext.h`: `IGeoOpsExtension` extension interface
- `geo_clustering.h`: DBSCAN and k-means spatial clustering (CPU)
