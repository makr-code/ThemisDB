# ThemisDB Geo Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The geo module provides geospatial query and indexing runtime surfaces for ThemisDB, including CPU/GPU spatial operations, indexing, GeoJSON geometry handling, clustering, raster workflows, temporal-spatial querying, and tile integration.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| cpu_backend.cpp | CPU exact/approximate spatial execution paths |
| boost_cpu_exact_backend.cpp | boost-based exact spatial backend |
| gpu_backend_stub.cpp | GPU dispatch and safe CPU fallback |
| gpu_backend_cuda.cu | CUDA geo kernel execution paths |
| gpu_backend_hip.cpp | HIP/ROCm geo backend paths |
| gpu_backend_production.cpp | production GPU backend wrapper |
| geo_rtree.cpp | R-tree indexing runtime |
| rtree_cursor.cpp | cursor-based geo index access |
| geo_json_geometry.cpp | GeoJSON geometry type hierarchy and validation |
| spatial_join.cpp | spatial join execution paths |
| spatial_join_filter.cpp | composable spatial join filters |
| geo_clustering.cpp | geo clustering algorithms |
| raster.cpp | raster query implementation |
| raster_query_interface.cpp | typed raster query interface |
| temporal_spatial_query.cpp | temporal-spatial query execution |
| temporal_spatial_query_builder.cpp | temporal-spatial query builder |
| tile_server.cpp | tile integration runtime |
| device_detector.cpp | runtime device/capability detection |
| geo_faiss_knn.cpp | FAISS-based geo k-NN bridge |

## Scope

In scope:
- spatial geometry/query/index operations on CPU and GPU backends
- GeoJSON geometry parsing/validation and spatial join/clustering/raster/temporal features
- backend selection, fallback, and device capability detection

Out of scope:
- navigation/routing engines
- external map service ownership outside tile integration contracts
- unrelated non-geospatial domain logic

## Runtime Behavior and Limits

- backend behavior depends on compile/runtime device capabilities and feature flags.
- GPU paths may fall back to CPU under unsupported/degraded conditions.
- exact/approximate precision mode affects performance and result behavior trade-offs.

## Sourcecode Verification (Module: geo/readme)

- Verified files:
  - src/geo/cpu_backend.cpp
  - src/geo/boost_cpu_exact_backend.cpp
  - src/geo/gpu_backend_stub.cpp
  - src/geo/gpu_backend_cuda.cu
  - src/geo/gpu_backend_hip.cpp
  - src/geo/gpu_backend_production.cpp
  - src/geo/geo_rtree.cpp
  - src/geo/rtree_cursor.cpp
  - src/geo/geo_json_geometry.cpp
  - src/geo/spatial_join.cpp
  - src/geo/spatial_join_filter.cpp
  - src/geo/geo_clustering.cpp
  - src/geo/raster.cpp
  - src/geo/raster_query_interface.cpp
  - src/geo/temporal_spatial_query.cpp
  - src/geo/temporal_spatial_query_builder.cpp
  - src/geo/tile_server.cpp
  - src/geo/device_detector.cpp
  - src/geo/geo_faiss_knn.cpp
- Verified behavior surfaces:
  - CPU/GPU spatial backends with deterministic fallback behavior
  - indexing, geometry, join, clustering, raster, and temporal-spatial paths
  - runtime device/precision integration boundaries
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md