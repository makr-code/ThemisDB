> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Geo Module

All notable changes to the Geo module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Spherical geometry support (WGS-84 ellipsoid) — Issue #1744
- GPU-accelerated DBSCAN / k-means clustering — Target: v2.3.0
- CUDA kernels for ST_BUFFER, ST_UNION, ST_DIFFERENCE on GPU — Target: v2.2.0

## [2.5.0] — 2026-04-15
### Added
- **Pull-based R-tree cursor API** (`include/geo/rtree_cursor.h`, `src/geo/rtree_cursor.cpp`):
  - `CursorStatus` enum (`OK`, `END`, `STALE`); `GeoIndexEntry` value type
  - `IRTreeCursor` abstract interface: `next(GeoIndexEntry&)`, `estimatedResultCount()`
  - `IGeoIndex` abstract interface: `openRangeCursor(MBR)`, `openKNNCursor(Coordinate, k)`, `insert()`, `bulkLoad()`, `clear()`, `size()`
  - `GeoRTreeIndex` concrete implementation wrapping `GeoRTree`; version counter invalidates open cursors on mutation
- **Fluent temporal-spatial query builder** (`include/geo/temporal_spatial_query_builder.h`, `src/geo/temporal_spatial_query_builder.cpp`):
  - `TimeWindowType` enum: `POINT_IN_TIME`, `INTERVAL`, `SLIDING_WINDOW`
  - `BuiltTemporalSpatialQuery` immutable value type; `execute(SystemVersionedTable)` method
  - `ITemporalSpatialQueryBuilder` abstract interface; `TemporalSpatialQueryBuilder` concrete implementation with `reset()` support
  - `build()` throws `std::logic_error` if temporal or spatial constraints are absent
- **Typed raster query interface** (`include/geo/raster_query_interface.h`, `src/geo/raster_query_interface.cpp`):
  - `RasterConfig` with configurable `maxTileSizeBytes()` (default 64 MiB)
  - `RasterStatus`: `OK`, `NOT_SUPPORTED`, `TILE_TOO_LARGE`, `INVALID_KEY`, `BACKEND_ERROR`, `INVALID_BBOX`
  - `IRasterQueryInterface` abstract interface; `RasterGridQueryImpl` (full, behind `THEMIS_ENABLE_RASTER`); `NoOpRasterQueryImpl` (always available, returns `NOT_SUPPORTED`)
- **GeoJSON geometry class hierarchy** (`include/geo/geo_json_geometry.h`, `src/geo/geo_json_geometry.cpp`):
  - `CrsId` enum; `BBox` struct; `ValidationError` and `ValidationResult`
  - `IGeoJSONGeometry` abstract base; concrete: `GeoPoint`, `GeoLineString`, `GeoPolygon` (right-hand-rule enforcement), `GeoMultiPolygon`, `GeoGeometryCollection`
- **Composable spatial join filters** (`include/geo/spatial_join_filter.h`, `src/geo/spatial_join_filter.cpp`):
  - `ISpatialJoinFilter` abstract interface; `IntersectsFilter`, `ContainsFilter`, `WithinFilter`, `TouchesFilter`, `DWithinFilter` (Haversine distance)
  - Logical combinators: `AndFilter`, `OrFilter`, `NotFilter`
  - `SpatialJoinFilter` factory namespace

## [2.3.0] — 2026-04-04
### Added
- **Full GeoJSON RFC 7946 parsing**: `EWKBParser::parseGeoJSON()` now handles all seven RFC 7946
  geometry types: `Point`, `MultiPoint`, `LineString`, `MultiLineString`, `Polygon`,
  `MultiPolygon`, and `GeometryCollection` (including 3D variants with Z coordinates).
- **GeoJSON serialization**: `EWKBParser::toGeoJSON()` serializes all seven geometry types.
- **EWKB extension**: `parse()` and `serialize()` now support all geometry types (types 4–7).
- **GeometryCollection recursion**: Parsed recursively up to depth 8 to prevent stack overflow
  on adversarial input.
- **computeMBR() / computeCentroid()**: Now recurse into nested sub-geometries.
- **WGS84 coordinate range validation**: Longitude in [-180, 180] and latitude in [-90, 90];
  invalid coordinates throw `std::runtime_error`. Compile with `-DTHEMIS_GEO_COMPAT_LAX` to
  skip during a migration window.
- **In-memory R-tree spatial index** (`include/geo/geo_rtree.h`, `src/geo/geo_rtree.cpp`):
  - `GeoRTree` class for `GeometryInfo` objects enabling sub-linear `intersects` and `contains` queries.
  - When compiled with `THEMIS_GEO_BOOST_BACKEND` and Boost.Geometry headers present, uses
    `boost::geometry::index::rtree` with `rstar<16>` splitting strategy.
  - Without Boost, automatically falls back to an O(n) linear MBR scan — semantically identical,
    no dependency required.
  - `bulkLoad(entries)` uses STR (Sort-Tile-Recursive) packing via the Boost bulk-insert constructor.

## [2.2.0] — 2026-03-21
### Added
- English documentation in `docs/en/geo/README.md` covering all components, API reference, AQL geo functions, configuration, architecture, and known limitations (Issue #1749)
- Comprehensive CI workflow `geo-module-ci.yml` covering 19 focused test targets across the entire geo module

## [1.7.0] — 2026-03-09
### Added
- DBSCAN and k-means geo-point clustering: `GeoCluster` API (`src/geo/geo_clustering.cpp`) (Issue #1747)
- Raster data queries: elevation sampling, bounding box extraction, Gaussian KDE heatmaps (`src/geo/raster.cpp`, `include/geo/raster.h`) (Issue #1745)
- Temporal-spatial queries: location at time T, entities within distance at time T (`src/geo/temporal_spatial_query.cpp`) (Issue #1746)
- Tile server integration for map visualization (`src/geo/tile_server.cpp`) (Issue #1748)

## [1.6.0] — 2026-02-01
### Added
- Spatial JOIN: find all point pairs within configurable distance threshold (`src/geo/spatial_join.cpp`) (Issue #1740)
- R-tree spatial index integration for CPU backend sub-linear query performance (Issue #1741)
- ROCm/HIP GPU backend for AMD hardware (`src/geo/gpu_backend_hip.cpp`) (Issue #1743)
- `ST_UNION` and `ST_DIFFERENCE` geometry operations on CPU-exact, Boost, and GPU-fallback backends; exposed as AQL functions (Issue #1739)

## [1.5.0] — 2026-01-10
### Added
- Full GeoJSON RFC 7946 spec coverage: all 7 geometry types including `GeometryCollection` and `MultiPolygon` (Issue #1737)
- `ST_BUFFER`: expand geometry by a fixed distance on CPU-exact and Boost backends; GPU backend delegates to CPU with audit log (Issue #1738)
- CUDA kernel dispatch for distance and containment: `gpu_backend_cuda.cu` integrating `cuda/geo_kernels.cu` (Issue #1752)
- GPU backend production path (`src/geo/gpu_backend_production.cpp`)
- GPU kernel dispatcher CPU fallback (`src/geo/gpu_kernel_dispatcher_cpu.cpp`)
- Structured audit log for GPU/CPU backend switches

## [1.0.0] — 2024-01-01
### Added
- CPU-based geospatial backend using Boost.Geometry for exact calculations (`src/geo/cpu_backend.cpp`)
- Boost.Geometry exact CPU backend (`src/geo/boost_cpu_exact_backend.cpp`)
- GPU-accelerated geospatial backend with automatic CPU fallback and circuit-breaker (`src/geo/gpu_backend_stub.cpp`)
- 2D and 3D spatial query support: contains, intersects, distance
- S2 cell indexing support and H3 hexagonal grid indexing
- Runtime GPU device discovery and capability reporting (`src/geo/device_detector.cpp`)
