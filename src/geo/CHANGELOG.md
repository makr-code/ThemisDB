> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Geo Module

All notable changes to the geo module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified geo benchmark symbols across geo CPU/GPU, index, join, hybrid, DBSCAN, and GeoJSON parse suites.

## [2.5.0] - 2026-04-15

### Added
- cursor API, temporal builder, typed raster interface, geometry hierarchy, and composable spatial join filters.

## [2.3.0] - 2026-04-04

### Added
- full GeoJSON RFC coverage and expanded spatial index handling.

## [1.7.0] - 2026-03-09

### Added
- clustering, raster, temporal-spatial queries, and tile integration.

## [1.6.0] - 2026-02-01

### Added
- spatial join, R-tree index integration, and HIP backend support.

## [1.5.0] - 2026-01-10

### Added
- ST_BUFFER and CUDA dispatch expansion with production GPU paths.

## [1.0.0] - 2024-01-01

### Added
- foundational CPU/GPU geospatial backend and indexing capabilities.