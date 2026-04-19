> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Geospatial Module

Geospatial query processing and indexing implementation for ThemisDB.

## Module Purpose

Implements geospatial query processing and spatial indexing for ThemisDB, providing 2D/3D spatial queries, geometry operations, and GPU-accelerated spatial processing with CPU fallback.

## Subsystem Scope

**In scope:** CPU-based spatial query processing (contains, intersects, distance), S2/H3 cell indexing, GPU-accelerated backend (CUDA/ROCm) with CPU fallback, 2D and 3D geometry operations, full GeoJSON RFC 7946 parsing, R-tree spatial indexing, raster data queries, temporal-spatial queries, geo-clustering (DBSCAN, k-means), tile server integration.

**Out of scope:** Routing/navigation algorithms, full spherical WGS-84 ellipsoid geometry (planned), real-time streaming geo-updates.

## Relevant Interfaces

- `cpu_backend.cpp` — primary CPU-based geospatial backend; implements `CpuExactBackend` and `ApproximateCpuBackend`
- `boost_cpu_exact_backend.cpp` — Boost.Geometry exact computation (optional, requires `THEMIS_GEO_BOOST_BACKEND`)
- `gpu_backend_stub.cpp` — GPU dispatch orchestrator with circuit-breaker CPU fallback
- `gpu_backend_cuda.cu` — CUDA kernel dispatch for distance and containment (`THEMIS_GEO_CUDA=ON`)
- `gpu_backend_hip.cpp` — ROCm/HIP kernel dispatch for AMD hardware (`THEMIS_ENABLE_HIP=ON`)
- `gpu_backend_production.cpp` — production GPU backend wrapper (metrics, audit log)
- `geo_rtree.cpp` — R-tree spatial index for sub-linear CPU query performance
- `spatial_join.cpp` — spatial JOIN finding all pairs within a configurable distance threshold
- `geo_clustering.cpp` — DBSCAN and k-means clustering for geo point datasets
- `raster.cpp` — raster grid queries (elevation sampling, bbox extraction, Gaussian KDE heatmaps)
- `temporal_spatial_query.cpp` — location-at-time-T queries over versioned document tables
- `tile_server.cpp` — map tile server integration
- `device_detector.cpp` — runtime GPU device discovery and capability reporting
- `getBackendForPrecision(GeoPrecisionMode)` — factory to select exact or approximate backend at call-site

## Current Delivery Status

**Maturity:** 🟡 Beta — CPU and GPU-accelerated backends operational. CUDA kernels for distance/containment implemented; ST_BUFFER/UNION/DIFFERENCE CUDA kernels deferred to v2.2.0 (CPU fallback active).

## Components

- CPU-based geospatial backend (`cpu_backend.cpp`, `boost_cpu_exact_backend.cpp`)
- GPU-accelerated geospatial backend (`gpu_backend_stub.cpp`, `gpu_backend_cuda.cu`, `gpu_backend_hip.cpp`, `gpu_backend_production.cpp`)
- R-tree spatial index (`geo_rtree.cpp`)
- Spatial JOIN support (`spatial_join.cpp`)
- Geo-point clustering: DBSCAN and k-means (`geo_clustering.cpp`)
- Raster data queries (`raster.cpp`)
- Temporal-spatial queries (`temporal_spatial_query.cpp`)
- Tile server integration (`tile_server.cpp`)
- GPU device detection (`device_detector.cpp`)

## Features

- 2D and 3D spatial queries
- Geometry operations: contains, intersects, distance, ST_BUFFER, ST_UNION, ST_DIFFERENCE
- Full GeoJSON RFC 7946 support (all 7 geometry types including `GeometryCollection` and `MultiPolygon`)
- S2/H3 cell indexing support
- R-tree spatial index for sub-linear query performance on large collections
- GPU acceleration for large-scale queries (CUDA + ROCm/HIP; CPU fallback via circuit-breaker when no device is present)
- **Configurable precision mode**: `GeoPrecisionMode::Exact` (full geometric algorithms, no false positives) and `GeoPrecisionMode::Approximate` (MBR-based fast pre-filter, no false negatives); selected via `getBackendForPrecision()`
- Spatial JOIN (all pairs within distance threshold)
- Geo-point clustering: DBSCAN and k-means
- Raster data queries (elevation sampling, bounding-box extraction, Gaussian KDE heatmaps)
- Temporal-spatial queries (location at time T, entities within distance at time T)
- Tile server integration for map visualization

## Documentation

For geospatial documentation, see:
- [GPU Backend Runbook](../../docs/gpu_runbooks.md#6-gpu-geospatial-backend-issues) — device discovery, circuit-breaker fallback, metrics, audit log, on-call procedures
- [GPU Roadmap](../../docs/gpu_roadmap.md) — production readiness status, completed geo backend work, remaining CUDA/ROCm items
- [ROADMAP.md](ROADMAP.md) — module roadmap, completed phases, planned features
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) — planned enhancements beyond the roadmap (spherical geometry, precision mode, scientific references)

## Scientific References

1. Guttman, A. (1984). **R-Trees: A Dynamic Index Structure for Spatial Searching**. *Proceedings of the 1984 ACM SIGMOD International Conference on Management of Data*, 47–57. https://doi.org/10.1145/602259.602266

2. Haverkort, H. J., & van Walderveen, F. (2008). **Locality and Bounding-Box Quality of Two-Dimensional Space-Filling Curves**. *Computational Geometry*, 42(5), 341–352. https://doi.org/10.1016/j.comgeo.2008.09.007

3. Open Geospatial Consortium. (2010). **OpenGIS® Implementation Standard for Geographic Information – Simple Feature Access – Part 1: Common Architecture (Version 1.2.1)**. OGC 06-103r4. https://www.ogc.org/standards/sfa

4. Haran, M. (2011). **Gaussian Random Field Models for Spatial Data**. *Handbook of Markov Chain Monte Carlo*, 449–478. https://doi.org/10.1201/b10905-19

5. Brinkhoff, T., Kriegel, H.-P., & Seeger, B. (1993). **Efficient Processing of Spatial Joins Using R-Trees**. *Proceedings of the 1993 ACM SIGMOD International Conference on Management of Data*, 237–246. https://doi.org/10.1145/170035.170075

6. Ester, M., Kriegel, H.-P., Sander, J., & Xu, X. (1996). **A Density-Based Algorithm for Discovering Clusters in Large Spatial Databases with Noise**. *Proceedings of the 2nd International Conference on Knowledge Discovery and Data Mining (KDD-96)*, 226–231. https://dl.acm.org/doi/10.5555/3001460.3001507

7. Vincenty, T. (1975). **Direct and Inverse Solutions of Geodesics on the Ellipsoid with Application of Nested Equations**. *Survey Review*, 23(176), 88–93. https://doi.org/10.1179/sre.1975.23.176.88

8. Greiner, G., & Hormann, K. (1998). **Efficient Clipping of Arbitrary Polygons**. *ACM Transactions on Graphics*, 17(2), 71–83. https://doi.org/10.1145/274363.274364

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/geo/README.md`](../../include/geo/README.md) for the public API.
