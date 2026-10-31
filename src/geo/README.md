---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "L1"
SOT_Domain: "module-behavior"
---

<!-- Links: ARCHITECTURE.md · API.md · MODULE_GUIDE.md · ROADMAP.md -->

# ThemisDB Geo Module

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

## Quickstart: build and run a focused check

The integrated Geo implementation is compiled into the parent ThemisDB build;
`src/geo` does not provide a standalone server executable. The parent CMake can
select the optional external Geo plugin with `THEMIS_EXTERNALIZE_GEO_PLUGIN`
when its plugin feature and submodule are available; in that mode this
integrated-source guide is not evidence about the plugin implementation. From
the default integrated build tree with tests enabled, build and run the
GeoJSON geometry suite:

```sh
cmake --build <build-dir> --target module_geo_test_geo_json_geometry_focused
ctest --test-dir <build-dir> -R '^test_geo_json_geometry_GeoFocusedTests$' --output-on-failure
```

This validates the focused geometry test only. Use the root build and service
deployment guidance to launch ThemisDB; for geo-specific test coverage and
backend requirements, see [IMPLEMENTATION.md](./IMPLEMENTATION.md).

## Developer documentation

| Document | Use it for |
|---|---|
| [ARCHITECTURE.md](./ARCHITECTURE.md) | Component boundaries, data flow, and failure domains |
| [RESEARCH.md](./RESEARCH.md) | Algorithm assumptions, evaluation evidence, and limits |
| [MODELING.md](./MODELING.md) | Geometry, coordinate, and query data models |
| [EMBEDDING.md](./EMBEDDING.md) | Geographic point projection used by the FAISS k-NN bridge |
| [IMPLEMENTATION.md](./IMPLEMENTATION.md) | Source layout, build variants, and focused validation |
| [API.md](./API.md) | Public geo interfaces and the binding contract |
| [MODULE_GUIDE.md](./MODULE_GUIDE.md) | Developer entry points and change workflow |
| [RUNBOOK.md](./RUNBOOK.md) | Incident diagnosis, recovery, and escalation |
| [OPERATIONS.md](./OPERATIONS.md) | Normal health, capacity, and performance operations |
| [TRUST.md](./TRUST.md) | Input, authorization, backend, and extension trust boundaries |
| [SECURITY.md](./SECURITY.md) | Module threat model and security-control summary |
| [ROADMAP.md](./ROADMAP.md) · [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md) | Planned work and research direction |

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

---

**AI provenance:** Exact model backend not verifiable from final document text alone.