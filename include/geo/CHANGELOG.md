<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Geo Module (Public Headers)

All notable changes to the Geo module public headers are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For implementation-level changes see `../../src/geo/CHANGELOG.md`.

## [Unreleased]
- Spherical geometry support (WGS-84 ellipsoid) — Issue #1744
- GPU-accelerated DBSCAN / k-means headers (`geo_clustering.h` GPU variants) — Target: v2.3.0

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
