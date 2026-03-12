<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Geo Module

All notable changes to the Geo module are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Spherical geometry support (WGS-84 ellipsoid) — Issue #1744
- Configurable precision mode (exact vs. approximate) — Issue #1742 (partially complete)

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
