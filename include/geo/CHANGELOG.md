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
- `rtree_cursor.h` + `src/geo/rtree_cursor.cpp`: pull-based R-tree cursor API —
  `CursorStatus` enum (OK/END/STALE), `GeoIndexEntry` value type, `IRTreeCursor`
  abstract with `next()`/`estimatedResultCount()`, `IGeoIndex` abstract with
  `openRangeCursor(bbox)`/`openKNNCursor(point, k)`, `GeoRTreeIndex` concrete
  implementation with version-based STALE detection
- `temporal_spatial_query_builder.h` + `src/geo/temporal_spatial_query_builder.cpp`:
  fluent builder for time-windowed spatial queries — `TimeWindowType` enum
  (POINT_IN_TIME/INTERVAL/SLIDING_WINDOW), `BuiltTemporalSpatialQuery` immutable value
  type with `execute(table)`, `ITemporalSpatialQueryBuilder` abstract,
  `TemporalSpatialQueryBuilder` concrete with `reset()`
- `raster_query_interface.h` + `src/geo/raster_query_interface.cpp`:
  `IRasterQueryInterface` abstract, `RasterConfig::maxTileSizeBytes()` size guard,
  `RasterStatus` enum (OK/NOT_SUPPORTED/TILE_TOO_LARGE/INVALID_KEY/BACKEND_ERROR/INVALID_BBOX),
  `RasterResult` with band data/CRS/resolution metadata, `RasterGridQueryImpl` backed by
  `RasterGrid`, `NoOpRasterQueryImpl` no-op stub, `makeRasterQueryInterface()` factory
- 10 unit tests RTC-01..10 in `tests/geo/test_rtree_cursor.cpp`
  (CMake target: `test_rtree_cursor_focused`)
- 8 unit tests TSB-01..08 in `tests/geo/test_temporal_spatial_query_builder.cpp`
  (CMake target: `test_temporal_spatial_query_builder_focused`)
- 8 unit tests RQI-01..08 in `tests/geo/test_raster_query_interface.cpp`
  (CMake target: `test_raster_query_interface_focused`)
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
