> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/geo/ARCHITECTURE.md -->

# Geo Module — Public Header Architecture

**Module Path:** `include/geo/`
**Implementation:** `../../src/geo/`
**Canonical architecture doc:** [`../../src/geo/ARCHITECTURE.md`](../../src/geo/ARCHITECTURE.md)

---

## 1. Overview

`include/geo/` defines the **public spatial storage, indexing, query, and GPU-accelerated analytics API contract** for ThemisDB. The 17 headers cover GeoJSON geometry types, R-tree indexing and cursors, spatial joins, KNN with FAISS GPU acceleration, raster storage and query, geo math, clustering, device detection, temporal-spatial queries, and tile serving.

For runtime composition — R-tree node splits, FAISS index build/update, raster tile caching, and GPU dispatch internals — see:
→ [`../../src/geo/ARCHITECTURE.md`](../../src/geo/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Geometry and Core Types

| Header | Public Type | Purpose |
|--------|------------|---------|
| `geo_json_geometry.h` | `GeoJSONGeometry` | GeoJSON geometry parsing, representation, and serialisation |
| `geo_math.h` | `GeoMath` | Geodetic distance, bearing, and projection utilities |
| `geo_ops_ext.h` | `GeoOpsExt` | Extended spatial operations (buffer, simplify, convex hull) |

### 2.2 Indexing and Spatial Access

| Header | Public Type | Purpose |
|--------|------------|---------|
| `geo_rtree.h` | `GeoRTree` | R-tree spatial index for bounding-box and point lookups |
| `rtree_cursor.h` | `RTreeCursor` | Iterative cursor over R-tree range results |
| `spatial_backend.h` | `ISpatialBackend` | Backend abstraction for swappable spatial index implementations |

### 2.3 Spatial Joins and Filtering

| Header | Public Type | Purpose |
|--------|------------|---------|
| `spatial_join.h` | `SpatialJoin` | Spatial join between two geometry collections |
| `spatial_join_filter.h` | `SpatialJoinFilter` | Predicate-based filter applied during spatial joins |

### 2.4 GPU-Accelerated KNN and Clustering

| Header | Public Type | Purpose |
|--------|------------|---------|
| `geo_faiss_knn.h` | `GeoFAISSKNN` | FAISS-backed GPU-accelerated KNN for geospatial points |
| `geo_clustering.h` | `GeoClustering` | Spatial clustering (k-means / DBSCAN variants) |
| `gpu_kernel_dispatcher.h` | `GPUKernelDispatcher` | Device-agnostic GPU kernel dispatch for spatial operations |
| `device_detector.h` | `DeviceDetector` | CUDA / HIP / CPU capability detection for spatial backends |

### 2.5 Raster Storage and Query

| Header | Public Type | Purpose |
|--------|------------|---------|
| `raster.h` | `RasterStore` | Raster tile storage and band management |
| `raster_query_interface.h` | `RasterQueryInterface` | Query API for raster band extraction and resampling |

### 2.6 Temporal-Spatial Queries

| Header | Public Type | Purpose |
|--------|------------|---------|
| `temporal_spatial_query.h` | `TemporalSpatialQuery` | Combined spatial + bi-temporal query execution |
| `temporal_spatial_query_builder.h` | `TemporalSpatialQueryBuilder` | Builder for composing temporal-spatial query predicates |

### 2.7 Tile Serving

| Header | Public Type | Purpose |
|--------|------------|---------|
| `tile_server.h` | `TileServer` | XYZ/TMS tile serving for vector and raster data |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::geo` | All spatial geometry, indexing, query, and serving types |

---

## 4. Public Contract Notes

- Geometry headers follow the GeoJSON (RFC 7946) geometry model; callers should not assume internal coordinate precision.
- R-tree headers provide stable index-access contracts; node split strategies and page layout remain internal.
- `ISpatialBackend` defines the extension point for alternative spatial index implementations.
- GPU acceleration headers are conditional on device availability; `DeviceDetector` must be consulted before dispatching GPU kernels.
- Temporal-spatial headers integrate with `include/temporal/` period semantics for AS OF spatial queries.
- Tile-server header provides an XYZ/TMS-compatible serving contract for map consumer integrations.
