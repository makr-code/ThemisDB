# 🌍 Geo Module — Geospatial Query Processing

**Category:** Feature Module  
**Version:** v2.2.0  
**Status:** 🟡 Beta — CPU and GPU-accelerated backends operational  
**Last Updated:** 2026-04-06

---

## 📑 Table of Contents

- [📋 Overview](#-overview)
- [✨ Features & Highlights](#-features--highlights)
- [🏗️ Components](#️-components)
- [🔌 API Reference](#-api-reference)
- [📖 AQL Geo Functions](#-aql-geo-functions)
- [🚀 Quick Start](#-quick-start)
- [⚙️ Configuration](#️-configuration)
- [🔧 Architecture](#-architecture)
- [📊 Performance](#-performance)
- [🔒 Security](#-security)
- [⚠️ Known Limitations](#️-known-limitations)
- [📚 See Also](#-see-also)

---

## 📋 Overview

The Geo module implements geospatial query processing and spatial indexing for ThemisDB. It evaluates OGC-compliant spatial predicates over geometry objects stored in the database and provides:

- **Two-tier backend**: CPU (exact) and GPU (CUDA/ROCm) with automatic circuit-breaker fallback
- **Full GeoJSON RFC 7946 support** for all 7 geometry types
- **R-tree spatial index** for sub-linear query performance on large collections
- **S2 and H3 cell indexing** for hierarchical and hexagonal spatial queries
- **Configurable precision mode**: `Exact` (no false positives) or `Approximate` (fast MBR pre-filter)

### Main Components

| Component | Header | Source | Description |
|-----------|--------|--------|-------------|
| `ISpatialComputeBackend` | `spatial_backend.h` | — | Backend interface (CPU/GPU) |
| `IGeoRegistry` | `spatial_backend.h` | — | Plugin registry for backends |
| `GeoPrecisionMode` | `spatial_backend.h` | — | `Exact` / `Approximate` enum |
| `CpuExactBackend` | — | `cpu_backend.cpp` | CPU exact geometric algorithms |
| `BoostCpuExactBackend` | — | `boost_cpu_exact_backend.cpp` | Boost.Geometry exact backend |
| `GpuBackendStub` | — | `gpu_backend_stub.cpp` | GPU dispatcher + CPU fallback |
| `GpuBackendCuda` | — | `gpu_backend_cuda.cu` | CUDA kernel dispatch (NVIDIA) |
| `GpuBackendHip` | — | `gpu_backend_hip.cpp` | ROCm/HIP kernel dispatch (AMD) |
| `GpuBackendProduction` | — | `gpu_backend_production.cpp` | Production GPU wrapper (metrics, audit) |
| `GeoRtree` | `geo_rtree.h` | `geo_rtree.cpp` | R-tree spatial index |
| `SpatialJoin` | `spatial_join.h` | `spatial_join.cpp` | All-pairs distance-threshold join |
| `GeoCluster` | `geo_clustering.h` | `geo_clustering.cpp` | DBSCAN + k-means clustering |
| `RasterGrid` | `raster.h` | `raster.cpp` | Elevation, bbox, Gaussian KDE heatmaps |
| `TemporalSpatialQuery` | `temporal_spatial_query.h` | `temporal_spatial_query.cpp` | Location at time T queries |
| `TileServer` | `tile_server.h` | `tile_server.cpp` | Map tile server integration |
| `DeviceDetector` | `device_detector.h` | `device_detector.cpp` | GPU discovery and capability reporting |
| `GpuKernelDispatcher` | `gpu_kernel_dispatcher.h` | `gpu_kernel_dispatcher_cpu.cpp` | Kernel dispatch table |

**Total:** 10 headers, 14 source files, ~5 300 LOC

---

## ✨ Features & Highlights

### Spatial Predicates (OGC-compliant)
- **ST_CONTAINS** — point-in-polygon / geometry containment
- **ST_INTERSECTS** — geometry intersection check
- **ST_DISTANCE** — Haversine / geodesic distance (metres)
- **ST_WITHIN** — inverse of ST_CONTAINS
- **ST_BUFFER** — expand geometry by a fixed distance (CPU + Boost backends; GPU delegates to CPU)
- **ST_UNION** — union of two geometries (CPU + Boost backends)
- **ST_DIFFERENCE** — set-difference of two geometries (CPU + Boost backends)

### Indexing
- **R-tree** — hierarchical spatial index (Boost.Geometry R*-tree, lazy build)
- **S2 Cells** — Google S2 geometry for spherical hierarchy lookups
- **H3 Cells** — Uber H3 hexagonal grid for uniform spatial binning
- **Geohash** — string-based tile encoding

### GPU Acceleration
- **CUDA Backend** (`gpu_backend_cuda.cu`) — NVIDIA GPUs, enabled by `THEMIS_GEO_CUDA=ON`
- **ROCm/HIP Backend** (`gpu_backend_hip.cpp`) — AMD GPUs, enabled by `THEMIS_ENABLE_HIP=ON`
- **Circuit-breaker fallback** — automatic CPU fallback on any GPU error with structured audit log

### Advanced Features
- **Full GeoJSON RFC 7946** — all 7 geometry types: `Point`, `MultiPoint`, `LineString`, `MultiLineString`, `Polygon`, `MultiPolygon`, `GeometryCollection`
- **EWKB encoding** — PostGIS-compatible Extended Well-Known Binary format for geometry storage
- **Configurable precision mode** — `Exact` (full geometric algorithms, no false positives) vs. `Approximate` (MBR pre-filter, no false negatives); selected via `getBackendForPrecision()`
- **Spatial JOIN** — all point pairs within a configurable distance threshold
- **DBSCAN clustering** — density-based geo-point cluster detection
- **k-means clustering** — centroid-based geo-point partitioning with k-means++ seeding
- **Raster data queries** — elevation sampling, bounding-box extraction, Gaussian KDE heatmaps
- **Temporal-spatial queries** — location at time T, entities within distance at time T
- **Tile server integration** — MVT (Mapbox Vector Tiles) generation and routing
- **3D geometry support** — Z-coordinate for points, lines, and polygons
- **WGS-84 geodesic distance** — Vincenty inverse formula on the WGS-84 ellipsoid

---

## 🏗️ Components

### CPU Backends

#### `CpuExactBackend` (`cpu_backend.cpp`)
Primary CPU geospatial backend. Implements:
- Ray-casting algorithm for point-in-polygon containment
- Segment-intersection based polygon overlap checks
- Haversine and Vincenty geodesic distance calculations
- ST_BUFFER (geodesic-aware circular/polygon expansion)
- ST_UNION and ST_DIFFERENCE via Greiner-Hormann algorithm
- WGS-84 ellipsoid distance via `geodesicDistance(lat1, lon1, lat2, lon2)`

#### `BoostCpuExactBackend` (`boost_cpu_exact_backend.cpp`)
Higher-precision backend using `Boost.Geometry`. Enabled by `THEMIS_GEO_BOOST_BACKEND`. Provides:
- `bg::intersects()` / `bg::within()` for exact polygon intersection
- `bg::buffer()` with `join_round`/`end_round` strategies for smooth ST_BUFFER output
- `bg::union_()` / `bg::difference()` for set operations
- Falls back to `CpuExactBackend` for unsupported geometry types

### GPU Backends

#### `GpuBackendStub` (`gpu_backend_stub.cpp`)
Dispatch orchestrator with circuit-breaker:
1. Checks for a CUDA or HIP device via `DeviceDetector`
2. On success → dispatches to `GpuBackendCuda` or `GpuBackendHip`
3. On failure or unavailable device → records audit log entry → delegates to CPU

#### `GpuBackendCuda` (`gpu_backend_cuda.cu`)
CUDA kernel dispatch for distance and containment operations. Requires `THEMIS_GEO_CUDA=ON`.

#### `GpuBackendHip` (`gpu_backend_hip.cpp`)
ROCm/HIP kernel dispatch for AMD hardware. Requires `THEMIS_ENABLE_HIP=ON`.

#### `GpuBackendProduction` (`gpu_backend_production.cpp`)
Production GPU backend wrapper providing:
- Per-operation metrics (latency, throughput, GPU memory)
- Structured audit log for all GPU↔CPU switches
- Health-check and circuit-breaker state reporting

### Spatial Index

#### `GeoRtree` (`geo_rtree.h`, `geo_rtree.cpp`)
In-memory R-tree index for 2D bounding-box queries:
- Boost.Geometry R*-tree with lazy build (built on first spatial query)
- Thread-safe via read-write lock (many concurrent readers, exclusive writes)
- `insert()`, `remove()`, `queryBBox()`, `nearestNeighbors()` operations
- `bulkLoad()` for efficient cold-start via STR packing
- Reports memory usage via `memoryBytes()`

### Clustering

#### `dbscanCluster()` / `kmeansCluster()` (`geo_clustering.h`, `geo_clustering.cpp`)
```cpp
// DBSCAN: density-based clustering
DbscanConfig config;
config.epsilon_m  = 500.0;   // 500 m neighbourhood radius
config.min_points = 3;        // min core-point density

GeoClusterResult result = dbscanCluster(points, config);
// result.labels[i] ∈ {cluster_id ≥ 0} or kDbscanNoise (-1)

// k-means: centroid-based clustering with k-means++ seeding
KMeansConfig km;
km.k             = 5;
km.max_iterations = 100;
km.seed          = 42;        // 0 = first k distinct points as centroids

GeoClusterResult km_result = kmeansCluster(points, km);
// result.labels[i] ∈ [0, k)
```

### Raster Data Queries

#### `RasterGrid` (`raster.h`, `raster.cpp`)
```cpp
// Elevation sampling
RasterGrid dem = RasterGrid::fromFile("elevation.tiff");
double elevation_m = sampleAt(dem, lon, lat);

// Bounding-box extraction
MBR bbox = queryBBox(dem, min_lon, min_lat, max_lon, max_lat);

// Gaussian KDE heatmap
HeatmapResult heatmap = generateHeatmap(points, sigma_m, cell_size_m);
```

### Temporal-Spatial Queries

#### `TemporalSpatialQuery` (`temporal_spatial_query.h`, `temporal_spatial_query.cpp`)
```cpp
TemporalSpatialQuery tsq(db, "tracks");

// Where was entity "vehicle_42" at timestamp T?
auto loc = tsq.locationAt("vehicle_42", /*timestamp_ms=*/1711000000000LL);

// All entities within 2 km of (52.52, 13.40) at time T
auto nearby = tsq.entitiesWithinDistanceAt(
    /*lat=*/52.52, /*lon=*/13.40,
    /*radius_m=*/2000.0,
    /*timestamp_ms=*/1711000000000LL
);
```

---

## 🔌 API Reference

### Backend Factory Functions

```cpp
#include "geo/spatial_backend.h"

// CPU exact backend (full geometric algorithms, no false positives)
ISpatialComputeBackend* getCpuExactBackend();

// CPU approximate backend (MBR-based fast pre-filter, no false negatives)
ISpatialComputeBackend* getCpuApproximateBackend();

// Precision-mode based selection
ISpatialComputeBackend* getBackendForPrecision(GeoPrecisionMode mode);
// GeoPrecisionMode::Exact      → getCpuExactBackend()
// GeoPrecisionMode::Approximate → getCpuApproximateBackend()

// GPU backend (CUDA/HIP with automatic CPU fallback via circuit-breaker)
ISpatialComputeBackend* getGpuBackend();
```

### `ISpatialComputeBackend` Interface

```cpp
class ISpatialComputeBackend {
public:
    virtual const char* name() const noexcept = 0;
    virtual bool isAvailable() const noexcept = 0;

    // Batch intersects for candidate pre-filter
    virtual SpatialBatchResults batchIntersects(const SpatialBatchInputs& in) = 0;

    // Exact intersects check between two geometries
    virtual bool exactIntersects(const GeometryInfo& geom1,
                                  const GeometryInfo& geom2) = 0;

    // Expand geometry by distance_m metres
    virtual GeometryInfo stBuffer(const GeometryInfo& geom, double distance_m,
                                   int arc_points = 36);

    // Union of two geometries
    virtual GeometryInfo stUnion(const GeometryInfo& a, const GeometryInfo& b);

    // Set-difference of two geometries
    virtual GeometryInfo stDifference(const GeometryInfo& a, const GeometryInfo& b);

    // WGS-84 ellipsoid geodesic distance (metres) — Vincenty inverse formula
    virtual double geodesicDistance(double lat1, double lon1,
                                     double lat2, double lon2);
};
```

### `GeoRtree` Index API

```cpp
#include "geo/geo_rtree.h"

GeoRTree tree;

// Insert a geometry with an associated key
tree.insert("key_1", makePoint(13.40, 52.52));  // lon, lat

// Range query — returns all keys whose MBR intersects the query bbox
MBR query_box{13.0, 52.0, 14.0, 53.0};
std::vector<std::string> hits = tree.queryBBox(query_box);

// k nearest neighbours
auto nn = tree.nearestNeighbors(13.40, 52.52, /*k=*/5);

// Bulk load for cold-start performance
tree.bulkLoad(geometries_with_keys);

// Memory footprint
std::size_t bytes = tree.memoryBytes();
```

### `SpatialJoin` API

```cpp
#include "geo/spatial_join.h"

SpatialJoin join;
join.setDistanceThresholdM(1000.0);  // 1 km
join.setMaxPairs(1'000'000);

// Execute join — returns all pairs (i, j) with distance ≤ threshold
auto pairs = join.execute(collection_a, collection_b);

// Standalone Haversine helper (always available)
double dist_m = haversineDistanceM(lat1, lon1, lat2, lon2);
```

---

## 📖 AQL Geo Functions

The Geo module exposes the following AQL functions via the query engine:

```aql
// --- Distance ---
LET dist_m = ST_DISTANCE(doc.location, @center_point)
FILTER dist_m < 1000

// --- Containment ---
FILTER ST_CONTAINS(@polygon, doc.location)

// --- Intersection ---
FILTER ST_INTERSECTS(doc.geometry, @search_area)

// --- Buffer ---
LET buffered = ST_Buffer(doc.geometry, 500)
FILTER ST_INTERSECTS(buffered, @area)

// --- Union / Difference ---
LET merged   = ST_UNION(@polygon_a, @polygon_b)
LET clipped  = ST_DIFFERENCE(@polygon_a, @polygon_b)

// --- Radius search (combined distance + sort) ---
FOR doc IN locations
  LET dist = ST_DISTANCE(doc.point, @user_location)
  FILTER dist < @radius_m
  SORT dist ASC
  LIMIT 20
  RETURN {doc, distance_m: dist}

// --- Spatial JOIN ---
FOR a IN collection_a
  FOR b IN collection_b
    FILTER GEO_DISTANCE(a.location, b.location) <= @threshold_m
    RETURN {a: a._key, b: b._key}
```

---

## 🚀 Quick Start

### 1. Include Headers

```cpp
#include "geo/spatial_backend.h"   // ISpatialComputeBackend, getBackendForPrecision()
#include "geo/geo_rtree.h"          // GeoRTree
#include "geo/spatial_join.h"       // SpatialJoin, haversineDistanceM()
#include "geo/geo_clustering.h"     // dbscanCluster(), kmeansCluster()
#include "utils/geo/ewkb.h"         // GeometryInfo, GeometryType, Coordinate
```

### 2. Spatial Predicate Check

```cpp
using namespace themis::geo;

// Build two geometries
GeometryInfo polygon(GeometryType::Polygon);
polygon.rings.push_back({
    {-0.1, 51.5}, {0.1, 51.5}, {0.1, 51.6}, {-0.1, 51.6}, {-0.1, 51.5}
});

GeometryInfo point(GeometryType::Point);
point.coords.emplace_back(0.0, 51.55);  // lon, lat

// Check containment
auto* backend = getCpuExactBackend();
bool inside = backend->exactIntersects(polygon, point);
```

### 3. Nearest Neighbour Search via R-tree

```cpp
GeoRTree tree;
for (const auto& [key, geom] : pois) {
    tree.insert(key, geom);
}

// Find 5 nearest POIs to a query point
auto nearest = tree.nearestNeighbors(/*lon=*/13.40, /*lat=*/52.52, /*k=*/5);
```

### 4. DBSCAN Clustering

```cpp
std::vector<GeometryInfo> points = loadPoints();

DbscanConfig cfg;
cfg.epsilon_m  = 500.0;  // 500 m
cfg.min_points = 4;

GeoClusterResult result = dbscanCluster(points, cfg);
// result.labels: cluster ID per point (-1 = noise)
// result.num_clusters: total number of clusters found
```

### 5. GPU-Accelerated Batch Intersects

```cpp
auto* gpu = getGpuBackend();  // falls back to CPU if no device found

SpatialBatchInputs inputs;
inputs.count   = pairs.size();
inputs.geoms_a = polygons;
inputs.geoms_b = query_points;

SpatialBatchResults results = gpu->batchIntersects(inputs);
// results.mask[i] == 1 → i-th pair intersects
```

---

## ⚙️ Configuration

| Parameter | Default | Description |
|-----------|---------|-------------|
| `geo.backend` | `"cpu"` | Active backend: `"cpu"` or `"gpu"` (falls back to cpu if unavailable) |
| `geo.precision_mode` | `"exact"` | `"exact"` (full algorithms) or `"approximate"` (MBR pre-filter) |
| `geo.rtree.max_node_size` | `64` | R-tree node capacity |
| `geo.exact_mode` | `false` | Use Boost.Geometry exact arithmetic (requires `THEMIS_GEO_BOOST_BACKEND`) |
| `geo.gpu.batch_size` | `65536` | GPU kernel batch size (points per launch) |

### CMake Build Flags

| Flag | Default | Description |
|------|---------|-------------|
| `THEMIS_GEO_BOOST_BACKEND` | `OFF` | Enable Boost.Geometry exact CPU backend |
| `THEMIS_GEO_CUDA` | `OFF` | Enable CUDA kernel dispatch (NVIDIA) |
| `THEMIS_ENABLE_HIP` | `OFF` | Enable ROCm/HIP kernel dispatch (AMD) |
| `THEMIS_ENABLE_S2` | `OFF` | Enable Google S2 cell indexing |
| `THEMIS_ENABLE_H3` | `OFF` | Enable Uber H3 hexagonal grid indexing |
| `THEMIS_ENABLE_TILE_SERVER` | `OFF` | Enable map tile server integration |
| `THEMIS_GEO_COMPAT_LAX` | `0` | Skip WGS-84 coordinate range validation (migration window) |

---

## 🔧 Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│              Query Engine (ST_* function calls)                  │
│   ST_CONTAINS(polygon, point) / ST_DISTANCE(a, b) / ...         │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                   Geo Backend Dispatcher                         │
│  ┌─────────────────────────────┐  ┌──────────────────────────┐  │
│  │      GpuBackendStub         │  │    CpuExactBackend        │  │
│  │  CUDA/HIP kernels           │→ │  Boost.Geometry fallback  │  │
│  └──────────┬──────────────────┘  └──────────────────────────┘  │
│             │                                                     │
│  ┌──────────▼──────────┐  ┌──────────────────────────────────┐  │
│  │ gpu_backend_cuda.cu │  │ boost_cpu_exact_backend.cpp       │  │
│  │ gpu_backend_hip.cpp │  │ (Boost.Geometry exact backend)    │  │
│  └─────────────────────┘  └──────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                     R-Tree Index (geo_rtree.cpp)                 │
│           Spatial index for bounding-box pre-filtering          │
└──────────────────────────────────────────────────────────────────┘
```

### Data Flow

```
AQL: FOR doc IN locations
       FILTER ST_DISTANCE(doc.point, @center) < @radius
       │
       ▼ Query engine resolves ST_DISTANCE → geo module
       │
       ▼ R-tree index: bounding-box pre-filter (fast candidate set)
       │
       ▼ CPU / GPU backend: exact distance computation on candidates
       │
       ▼ Results filtered by radius → return to query engine
```

### GPU → CPU Fallback

```
GpuBackendStub: check for CUDA/HIP device via DeviceDetector
   ├─ device available → run CUDA kernel (gpu_backend_cuda.cu)
   │                   or HIP kernel  (gpu_backend_hip.cpp)
   └─ device unavailable / runtime error →
          circuit-breaker triggers (structured audit log entry)
              → GeoKernelFallbackDispatcher (src/acceleration/)
                  → CpuExactBackend
```

---

## 📊 Performance

| Operation | Dataset | Backend | Latency (p99) |
|-----------|---------|---------|---------------|
| ST_DISTANCE | 1M point pairs | CPU (Haversine) | ~80 ms |
| ST_CONTAINS | 100K points vs polygon | CPU (ray-cast) | ~25 ms |
| ST_INTERSECTS (R-tree) | 1M geometries | CPU + R-tree | ≤ 5 ms |
| Bulk R-tree insert | 1M geometries | CPU | ≤ 3 s |
| DBSCAN clustering | 10K points, ε=500 m | CPU | ≤ 5 s |
| k-means clustering | 100K points, k=10 | CPU | ≤ 2 s |
| GPU batchIntersects | 65K pairs | CUDA (RTX-class) | ≥ 8× vs CPU |

> Performance benchmarks are documented in detail in [geo_benchmarks.md](../../../docs/de/geo/geo_benchmarks.md).

---

## 🔒 Security

The geo module enforces the following security controls:

- **Geometry input validation**: all coordinates are validated for NaN/Inf and out-of-range WGS-84 values before any backend dispatch.
- **GeoJSON strict mode**: RFC 7946 parser rejects unknown geometry types and out-of-range coordinates. Use `-DTHEMIS_GEO_COMPAT_LAX=1` for a one-release migration window.
- **No code execution from geometry**: geometry operations are pure mathematical computations; no user-supplied code is evaluated.
- **GPU kernel bounds checking**: polygon vertex counts and batch sizes are validated before CUDA kernel launch to prevent buffer overflows.
- **Audit trail**: all GPU↔CPU backend switches are recorded in the structured audit log.
- **Tile server SSRF prevention**: tile coordinates (x, y, z) are validated as bounded integers; no external URL fetching.
- **Temporal-spatial GDPR note**: location history queries may constitute personal location data if linked to individual identities; apply collection-level schema masking.

For the full threat model see [`src/geo/SECURITY.md`](../../../src/geo/SECURITY.md).

---

## ⚠️ Known Limitations

| Limitation | Target |
|------------|--------|
| ST_BUFFER, ST_UNION, ST_DIFFERENCE on GPU use CPU fallback (no dedicated CUDA kernels) | v2.2.0 |
| DBSCAN and k-means use O(n²) brute-force distance; no GPU acceleration | v2.3.0 |
| Spherical WGS-84 ellipsoid geometry (full Vincenty/Karney model) is partially implemented | v2.5.0 |
| ROCm/HIP geo kernel dispatch requires `THEMIS_ENABLE_HIP=ON` and ROCm runtime | — |
| R-tree index is in-memory; rebuilt on startup (no persistence) | planned |

---

## 📚 See Also

| Document | Description |
|----------|-------------|
| [`src/geo/ARCHITECTURE.md`](../../../src/geo/ARCHITECTURE.md) | Component architecture, data flow, threading model |
| [`src/geo/ROADMAP.md`](../../../src/geo/ROADMAP.md) | Module roadmap and implementation phases |
| [`src/geo/CHANGELOG.md`](../../../src/geo/CHANGELOG.md) | Version history and notable changes |
| [`src/geo/FUTURE_ENHANCEMENTS.md`](../../../src/geo/FUTURE_ENHANCEMENTS.md) | Planned enhancements with scientific references |
| [`src/geo/SECURITY.md`](../../../src/geo/SECURITY.md) | Threat model and security controls |
| [`src/geo/AUDIT.md`](../../../src/geo/AUDIT.md) | Latest audit report |
| [`include/geo/README.md`](../../../include/geo/README.md) | Header file index |
| [`docs/de/geo/README.md`](../../de/geo/README.md) | German-language module overview |
| [`docs/de/geo/geo_architecture.md`](../../de/geo/geo_architecture.md) | Cross-cutting capability design |
| [`docs/de/geo/geo_benchmarks.md`](../../de/geo/geo_benchmarks.md) | Detailed performance benchmarks |
| [`docs/de/geo/geo_integration.md`](../../de/geo/geo_integration.md) | Integration guide (EWKB, hooks) |
| [`docs/troubleshooting/geo_troubleshooting.md`](../../troubleshooting/geo_troubleshooting.md) | Troubleshooting guide |

### Scientific References

1. Guttman, A. (1984). **R-Trees: A Dynamic Index Structure for Spatial Searching**. *Proc. ACM SIGMOD*, 47–57. https://doi.org/10.1145/602259.602266
2. Ester, M. et al. (1996). **A Density-Based Algorithm for Discovering Clusters in Large Spatial Databases with Noise**. *KDD-96*, 226–231. https://dl.acm.org/doi/10.5555/3001460.3001507
3. Vincenty, T. (1975). **Direct and Inverse Solutions of Geodesics on the Ellipsoid**. *Survey Review*, 23(176), 88–93. https://doi.org/10.1179/sre.1975.23.176.88
4. Brinkhoff, T. et al. (1993). **Efficient Processing of Spatial Joins Using R-Trees**. *Proc. ACM SIGMOD*, 237–246. https://doi.org/10.1145/170035.170075
5. Open Geospatial Consortium (2010). **OpenGIS Simple Feature Access — Part 1: Common Architecture** (v1.2.1). OGC 06-103r4.
