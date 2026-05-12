> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Geo Module Headers

This directory contains header files (.h) for the geo module.

## Purpose

Public interfaces and declarations for geo functionality. Implementation source files are in `../../src/geo/`.

## Public Header Entry Points

| Header | Description |
|--------|-------------|
| `spatial_backend.h` | Core backend API: `ISpatialComputeBackend`, `GeoPrecisionMode`, `getBackendForPrecision()`, GPU backend getters, backend stats/device JSON |
| `geo_ops_ext.h` | Optional extension interface `IGeoOpsExtension` for geometry operations (`ST_BUFFER`, `ST_UNION`, `ST_DIFFERENCE`) |
| `geo_rtree.h` | `GeoRTree` in-memory spatial index for MBR `intersects`/`contains` queries |
| `rtree_cursor.h` | Pull-based index API: `IRTreeCursor`, `IGeoIndex`, `GeoRTreeIndex`, `CursorStatus` |
| `gpu_kernel_dispatcher.h` | CUDA/HIP dispatch wrapper for geospatial batch kernels (distance/containment) |
| `spatial_join.h` | Spatial join API and Haversine helper for distance-threshold join processing |
| `spatial_join_filter.h` | Composable spatial predicates: `IntersectsFilter`, `ContainsFilter`, `WithinFilter`, `TouchesFilter`, `DWithinFilter` and combinators |
| `geo_clustering.h` | Geo clustering entry points (`dbscanCluster()`, `kmeansCluster()`) |
| `geo_faiss_knn.h` | FAISS-based geo k-NN/radius interface via ECEF projection |
| `geo_json_geometry.h` | RFC 7946 geometry OOP API (`IGeoJSONGeometry`, `GeoPoint`, `GeoPolygon`, `GeoMultiPolygon`, `ValidationResult`, `CrsId`) |
| `geo_math.h` | Geodesic helpers (`haversine`, bearing and related math primitives) |
| `raster.h` | Raster grid abstraction for elevation sampling, bbox extraction and heatmaps |
| `raster_query_interface.h` | Typed raster interface (`IRasterQueryInterface`, `RasterGridQueryImpl`, `NoOpRasterQueryImpl`, `RasterStatus`) |
| `temporal_spatial_query.h` | Temporal-spatial query execution API for versioned datasets |
| `temporal_spatial_query_builder.h` | Fluent query builder (`ITemporalSpatialQueryBuilder`, `TemporalSpatialQueryBuilder`, `BuiltTemporalSpatialQuery`) |
| `tile_server.h` | Tile server integration API |
| `device_detector.h` | Runtime GPU capability/discovery interface for geo backend routing |

## Runtime Behavior, Errors, and Limits

- `GeoPrecisionMode::Exact` uses full geometric checks; `GeoPrecisionMode::Approximate` uses MBR pre-filtering (faster, may include false positives).
- GPU backend selection is runtime-safe: unavailable/erroring devices fall back to CPU with circuit-breaker behavior.
- `IRasterQueryInterface` returns explicit `RasterStatus` values (`NOT_SUPPORTED`, `INVALID_BBOX`, `TILE_TOO_LARGE`, `BACKEND_ERROR`, ...).
- `IRTreeCursor::next()` returns `CursorStatus::STALE` if the underlying index changed while iterating.
- Current limits (also tracked in module docs): GPU DBSCAN defaults to finite max-n threshold; GPU polygon `ST_BUFFER`/`ST_UNION`/`ST_DIFFERENCE` still use CPU fallback paths.

## Configuration Options (Build/Runtime)

- `THEMIS_GEO_BOOST_BACKEND` — enable Boost.Geometry-backed exact operations where available.
- `THEMIS_GEO_CUDA` / `THEMIS_GEO_HIP` — enable CUDA/HIP geo kernel dispatch builds.
- `THEMIS_ENABLE_HIP` — enable ROCm/HIP backend integration.
- `THEMIS_ENABLE_RASTER` — enable full raster query implementation; otherwise `NoOpRasterQueryImpl` reports `NOT_SUPPORTED`.
- Runtime tuning references (`geo.backend`, `geo.precision_mode`, `geo.gpu.batch_size`) are documented in `../../src/geo/ARCHITECTURE.md`.

## Documentation

- `../../src/geo/README.md` — module overview and feature list
- `../../src/geo/ARCHITECTURE.md` — component architecture, data flow, threading model
- `../../src/geo/ROADMAP.md` — module roadmap and implementation phases
- `../../src/geo/FUTURE_ENHANCEMENTS.md` — planned enhancements with scientific references
- `../../docs/de/geo/` — German-language developer documentation

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "geo/spatial_backend.h"
#include "geo/rtree_cursor.h"
#include "geo/geo_json_geometry.h"
```

Minimal precision-mode backend selection:

```cpp
using namespace themis::geo;
auto* backend = getBackendForPrecision(GeoPrecisionMode::Exact);
```

## Troubleshooting

- Runtime operational issues and fallback behavior: [`../../docs/troubleshooting/geo_troubleshooting.md`](../../docs/troubleshooting/geo_troubleshooting.md)
- GPU backend runbook and on-call procedures: [`../../docs/de/features/gpu_runbooks.md`](../../docs/de/features/gpu_runbooks.md)

## Related Documentation

- [`../../src/geo/README.md`](../../src/geo/README.md) — module overview, component map, usage
- [`../../src/geo/ARCHITECTURE.md`](../../src/geo/ARCHITECTURE.md) — architecture, data flow, configuration and limits
- [`../../src/geo/ROADMAP.md`](../../src/geo/ROADMAP.md) — delivery status and phased roadmap
- [`../../src/geo/FUTURE_ENHANCEMENTS.md`](../../src/geo/FUTURE_ENHANCEMENTS.md) — planned enhancements and scientific references
- [`../../docs/en/geo/README.md`](../../docs/en/geo/README.md) — English geo module documentation
- [`../../docs/de/geo/README.md`](../../docs/de/geo/README.md) — German geo module documentation
