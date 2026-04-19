<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/geo/ -->

# Geo Module — Public Header Architecture
**Version:** 2.2.0
**Module Path:** `include/geo/`
**Implementation:** `../../src/geo/`

---

## Overview

The Geo module provides public headers for geospatial query operations, spatial indexing, raster data, tile serving, temporal-spatial queries, and GPU-accelerated processing. It supports pluggable spatial backends and the R-tree spatial index.

## Design Principles

- **Pluggable Backends** — `ISpatialComputeBackend` and `IGeoRegistry` abstract all spatial operations; CPU and GPU backends interchangeable.
- **GPU Acceleration** — `GpuKernelDispatcher` dispatches CUDA kernels for distance and containment; `GeoDeviceDetector` auto-selects best available device.
- **Temporal-Spatial** — `TemporalSpatialQuery` combines time-window filtering with spatial predicates.
- **Standards Compliant** — Follows OGC Simple Features and WGS-84 coordinate reference system.

## Interface Inventory

| Header | Classes / Structs | Purpose |
|--------|-------------------|---------|
| `geo_rtree.h` | `GeoRTree` | R-tree spatial index for bounding-box and point queries |
| `spatial_backend.h` | `ISpatialComputeBackend`, `IGeoRegistry`, `SpatialBatchInputs`, `SpatialBatchResults` | Pluggable spatial compute abstraction |
| `spatial_join.h` | `SpatialJoinIterator`, `SpatialJoinConfig`, `SpatialJoinPair` | Streaming spatial join between two geometry collections |
| `geo_ops_ext.h` | `IGeoOpsExtension` | Extension point for custom geo operations |
| `geo_clustering.h` | `IGeoOpsExtension`, `GeoClusterResult`, `DbscanConfig`, `KMeansConfig` | DBSCAN and k-means spatial clustering |
| `gpu_kernel_dispatcher.h` | `GpuKernelDispatcher` | CUDA kernel dispatch for geospatial operations |
| `device_detector.h` | `GeoDeviceDetector`, `GeoDeviceCapability` | Auto-detect CUDA vs. CPU backend |
| `raster.h` | `RasterGrid`, `RasterSampleResult`, `HeatmapConfig` | Raster grid operations and heatmap generation |
| `tile_server.h` | `TileCoord` | Map tile coordinate utilities |
| `temporal_spatial_query.h` | `TemporalSpatialQuery` | Time-window + spatial predicate queries |

## References

- Implementation details: `../../src/geo/`
- AQL geo functions: `docs/en/geo/README.md`
- GPU kernel details: `../../src/geo/ARCHITECTURE.md`
