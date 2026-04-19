> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Geo Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/geo/`

---

## 1. Overview

The Geo module provides geospatial query processing and spatial indexing for ThemisDB.
It evaluates OGC-compliant spatial predicates (ST_Contains, ST_Intersects, ST_Distance,
ST_Within, etc.) over geometry objects (points, linestrings, polygons) stored in the database,
and integrates with the acceleration module for GPU-accelerated spatial joins on large datasets.

---

## 2. Design Principles

- **Two-Tier Backend** – a CPU backend provides full correctness; a GPU backend (CUDA
  and ROCm/HIP) provides throughput at scale. Requests fall back to CPU automatically
  via circuit-breaker on any CUDA/HIP runtime error.
- **OGC Compliance** – spatial predicates follow the OGC Simple Feature Access standard
  (Part 1: Common Architecture).
- **S2/H3 Cell Indexing** – geospatial data is indexed using Google S2 or Uber H3 cells
  for efficient range queries on the sphere.
- **Exact vs. Approximate** – callers choose a precision mode via `GeoPrecisionMode`:
  `Exact` runs full ray-casting/segment-intersection algorithms (no false positives);
  `Approximate` uses MBR (bounding-box) overlap for O(1) conservative checks
  (no false negatives; may produce false positives, safe as a spatial pre-filter).
  `boost_cpu_exact_backend.cpp` provides a higher-precision Boost.Geometry path when
  available.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `cpu_backend.cpp` | Primary CPU geospatial backend (contains, intersects, distance, ST_BUFFER, ST_UNION, ST_DIFFERENCE) |
| `boost_cpu_exact_backend.cpp` | Boost.Geometry exact computation backend |
| `geo_rtree.cpp` | R-tree spatial index for 2D bounding-box queries |
| `gpu_backend_stub.cpp` | GPU dispatch orchestrator with circuit-breaker CPU fallback |
| `gpu_backend_cuda.cu` | CUDA kernel dispatch for distance and containment (requires `THEMIS_GEO_CUDA=ON`) |
| `gpu_backend_hip.cpp` | ROCm/HIP kernel dispatch for AMD hardware (requires `THEMIS_ENABLE_HIP=ON`) |
| `gpu_backend_production.cpp` | Production GPU backend wrapper (metrics, structured audit log) |
| `gpu_kernel_dispatcher_cpu.cpp` | CPU-side GPU kernel dispatcher (fallback path) |
| `spatial_join.cpp` | Spatial JOIN: all point pairs within a configurable distance threshold |
| `geo_clustering.cpp` | DBSCAN and k-means clustering for geo point datasets |
| `raster.cpp` | Raster grid queries: elevation sampling, bbox extraction, Gaussian KDE heatmaps |
| `temporal_spatial_query.cpp` | Location-at-time-T queries over `SystemVersionedTable` |
| `tile_server.cpp` | Map tile server integration |
| `device_detector.cpp` | Runtime GPU device discovery and compute-capability/VRAM reporting |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│              Query Engine (ST_* function calls)                  │
│   ST_Contains(polygon, point) / ST_Distance(a, b) / ...         │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                   Geo Backend Dispatcher                         │
│                                                                  │
│  ┌─────────────────────────────────┐  ┌─────────────────────┐   │
│  │        GPU Backend Stub         │  │    CPU Backend      │   │
│  │  (CUDA/HIP kernels implemented) │→ │  (fallback)         │   │
│  └──────────┬──────────────────────┘  └──────────┬──────────┘   │
│             │                                     │              │
│  ┌──────────▼──────────┐         ┌────────────────▼──────────┐  │
│  │  gpu_backend_cuda.cu│         │ Boost.Geometry Exact       │  │
│  │  gpu_backend_hip.cpp│         │ Backend                    │  │
│  └─────────────────────┘         └───────────────────────────┘  │
└──────────────────────────────────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                     R-Tree Index (geo_rtree.cpp)                 │
│           Spatial index for bounding-box pre-filtering          │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Spatial Query Execution

```
AQL: FOR doc IN locations
       FILTER ST_Distance(doc.point, @center) < @radius
       RETURN doc
    │
    ▼
Query engine resolves ST_Distance → geo module
    │
    ▼
R-tree index: bounding-box pre-filter (fast candidate set)
    │
    ▼
CPU / GPU backend: exact distance computation on candidates
    │
    ▼
Results filtered by radius → return to query engine
```

### 4.2 GPU → CPU Fallback

```
GPU backend stub: check for CUDA/HIP device
    ├─ device available → run CUDA kernel (gpu_backend_cuda.cu)
    │                   or HIP kernel (gpu_backend_hip.cpp)
    └─ device unavailable / runtime error →
           circuit-breaker triggers (structured audit log entry)
               → GeoKernelFallbackDispatcher (src/acceleration/)
                   → cpu_backend.cpp
```

Note: ST_BUFFER, ST_UNION, and ST_DIFFERENCE currently use the CPU fallback path for GPU requests; dedicated CUDA kernels for these set operations are deferred to v2.2.0.

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Called by** | `src/query/` | ST_* function evaluation |
| **Uses** | `src/acceleration/` | `GeoKernelFallbackDispatcher` for GPU dispatch |
| **Uses** | `src/index/` | Spatial index registration |
| **Provides to** | `src/query/` | Spatial predicate results |

---

## 6. Threading & Concurrency Model

- CPU backend functions are stateless and safe for concurrent invocation.
- `GeoRtree` (R-tree index) uses a read-write lock: many concurrent readers, exclusive writes.
- GPU kernel dispatch uses the acceleration module's thread-safe backend registry.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| R-tree pre-filtering | Reduces exact distance computation to small candidate set |
| S2/H3 cell indexing | O(1) cell lookup for point-in-region queries |
| GPU batch processing | Planned: batch point-in-polygon over 1M+ points on GPU |
| Boost.Geometry exact | Used only when precision is required; fast path uses float |

---

## 8. Security Considerations

- Input geometry is validated before processing to prevent malformed geometry crashes.
- Coordinate values are validated for NaN and infinity to prevent FPE.
- No user-supplied code is executed; geometry operations are pure math.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `geo.backend` | "cpu" | Backend: "cpu", "gpu" (falls back to cpu if unavailable) |
| `geo.precision_mode` | "exact" | Precision for spatial predicates: "exact" (full geometric algorithms, `getCpuExactBackend()`) or "approximate" (MBR-based pre-filter, `getCpuApproximateBackend()`). Select via `getBackendForPrecision(GeoPrecisionMode)`. |
| `geo.rtree.max_node_size` | 64 | R-tree node capacity |
| `geo.exact_mode` | false | Use Boost.Geometry exact arithmetic (requires `THEMIS_GEO_BOOST_BACKEND`) |
| `geo.gpu.batch_size` | 65536 | GPU kernel batch size (points per launch) |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Invalid geometry (NaN/Inf) | Return structured error; reject query |
| Unsupported geometry type | Return `ST_NULL` / structured error |
| GPU kernel failure | `GeoKernelFallbackDispatcher` → CPU fallback |
| R-tree overflow | Trigger split and rebalance |

---

## 11. Known Limitations & Future Work

- ST_BUFFER, ST_UNION, and ST_DIFFERENCE use CPU fallback for the GPU path; dedicated CUDA kernels for these set operations are deferred to v2.2.0.
- DBSCAN and k-means clustering use O(n²) brute-force distance computation; spatial-index acceleration and GPU-accelerated paths are deferred to a future release.
- 3D geometry operations (Z-coordinate) are partially implemented.
- Spherical WGS-84 ellipsoid geometry (as opposed to the current planar/Haversine approximation) is planned.
- Routing/navigation algorithms (shortest path via road network) are out of scope.

---

## 12. References

- `src/geo/README.md` — module overview and feature list
- `src/geo/FUTURE_ENHANCEMENTS.md` — planned features with IEEE scientific references
- `src/geo/ROADMAP.md` — module roadmap and implementation phases
- `docs/gpu_roadmap.md` — GPU integration status
- `docs/gpu_runbooks.md#6-gpu-geospatial-backend-issues` — operations runbook
- `docs/de/geo/` — German-language developer documentation
- `ARCHITECTURE.md` (root) — full system architecture
