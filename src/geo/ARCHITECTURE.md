# Geo Module — Architecture Guide

**Version:** 1.0  
**Last Updated:** 2026-02-24  
**Module Path:** `src/geo/`

---

## 1. Overview

The Geo module provides geospatial query processing and spatial indexing for ThemisDB.
It evaluates OGC-compliant spatial predicates (ST_Contains, ST_Intersects, ST_Distance,
ST_Within, etc.) over geometry objects (points, linestrings, polygons) stored in the database,
and integrates with the acceleration module for GPU-accelerated spatial joins on large datasets.

---

## 2. Design Principles

- **Two-Tier Backend** – a CPU backend provides full correctness; a GPU backend (stub,
  in progress) provides throughput at scale. Requests fall back to CPU automatically.
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
| `cpu_backend.cpp` | Primary CPU geospatial backend (contains, intersects, distance) |
| `boost_cpu_exact_backend.cpp` | Boost.Geometry exact computation backend |
| `geo_rtree.cpp` | R-tree spatial index for 2D bounding-box queries |
| `gpu_backend_stub.cpp` | GPU backend stub with automatic CPU fallback |

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
│  │   (CUDA kernels – in progress)  │→ │  (fallback)         │   │
│  └─────────────────────────────────┘  └──────────┬──────────┘   │
│                                                   │              │
│                           ┌───────────────────────┴──────────┐  │
│                           │  Boost.Geometry Exact Backend     │  │
│                           │  (high-precision operations)      │  │
│                           └───────────────────────────────────┘  │
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
GPU backend stub: check for CUDA device
    ├─ device available → run CUDA kernel (planned)
    └─ device unavailable / error → 
           GeoKernelFallbackDispatcher (src/acceleration/)
               → cpu_backend.cpp
```

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

- GPU backend (`gpu_backend_stub.cpp`) is a stub; CUDA kernels are planned.
- Full GeoJSON RFC 7946 parsing is planned (currently partial).
- `ST_Buffer`, `ST_Union`, `ST_Difference` are planned.
- 3D geometry operations (Z-coordinate) are partially implemented.
- Routing/navigation algorithms (shortest path via road network) are out of scope.

---

## 12. References

- `src/geo/README.md` — module overview
- `docs/geospatial_future_enhancements.md` — planned features
- `docs/gpu_roadmap.md` — GPU integration status
- `docs/gpu_runbooks.md#6-gpu-geospatial-backend-issues` — operations runbook
- `ARCHITECTURE.md` (root) — full system architecture
