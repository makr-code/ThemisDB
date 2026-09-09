# Architecture - Geo Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The geo module composes CPU/GPU spatial backends, indexing structures, geometry processing, and advanced geo workflows into a bounded runtime contract for geospatial workloads in ThemisDB.

## Main Execution Planes

1. Backend execution plane
- exact/approximate CPU paths and optional Boost exact behavior
- GPU execution with CUDA/HIP paths and deterministic CPU fallback

2. Spatial index and geometry plane
- R-tree and cursor-based index access
- GeoJSON geometry hierarchy, validation, and geometric operations

3. Query and analytics plane
- spatial joins and composable filters
- clustering, raster, temporal-spatial, and k-NN bridge workflows

4. Runtime integration plane
- device detection and backend selection
- tile and runtime observability integration surfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| backend contract | deterministic CPU/GPU selection and fallback semantics |
| indexing/geometry contract | explicit spatial indexing and geometry-validation behavior |
| query contract | bounded join/clustering/raster/temporal execution semantics |
| integration contract | explicit capability detection and runtime bridge behavior |

## Failure Semantics

- unsupported or degraded GPU paths fail over to CPU where supported.
- invalid geometry inputs fail with explicit validation errors.
- unavailable optional features surface deterministic non-silent failure behavior.

## Sourcecode Verification (Module: geo/architecture)

- Verified files:
  - src/geo/cpu_backend.cpp
  - src/geo/gpu_backend_stub.cpp
  - src/geo/gpu_backend_cuda.cu
  - src/geo/gpu_backend_hip.cpp
  - src/geo/geo_rtree.cpp
  - src/geo/geo_json_geometry.cpp
  - src/geo/spatial_join.cpp
  - src/geo/raster.cpp
  - src/geo/temporal_spatial_query.cpp
- Verified architecture claims:
  - explicit backend/indexing/query/integration planes
  - bounded deterministic failure behavior for invalid/degraded paths
  - module-local ownership of geospatial runtime orchestration

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| storage | `include/storage/` | Reads and writes geometry features and spatial datasets |
| index | `include/index/spatial_index.h` | R-tree and spatial index structures for cursor-based lookups |
| acceleration | `include/acceleration/gpu_backend.h` | Optional GPU backend dispatch for CUDA/HIP spatial computations |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `server/spatial_api_handler.h` | Exposes spatial query, join, and tile APIs over HTTP/wire |
| query | `include/geo/` (geospatial_cost_model.h, spatial_backend.h) | Query planner uses geo cost-model and backend selection for spatial sub-plans |

## Integration Points

### Critical Integration: Index Spatial Structures
**Files:** `src/geo/geo_rtree.cpp` ↔ `index/spatial_index.h`
**Contract:** Geo module creates and queries R-tree instances via the index module contract; index module owns cursor lifecycle.
**Thread Safety:** R-tree reads are concurrency-safe via index module locking; geo caller must not modify cursor state across threads.

### Critical Integration: Query Geospatial Cost Model
**Files:** `src/geo/spatial_join.cpp` ↔ `query/geospatial_cost_model.h`
**Contract:** Query planner queries geo cost-model for join and filter cardinality estimates; estimates must be deterministic for fixed inputs.
**Thread Safety:** Cost model is read-only after initialisation; concurrent queries are safe.

### Critical Integration: Acceleration GPU Backend
**Files:** `src/geo/gpu_backend_cuda.cu`, `src/geo/gpu_backend_hip.cpp` ↔ `acceleration/gpu_backend.h`
**Contract:** Geo module delegates compute kernels to the acceleration layer; fallback to CPU path is mandatory when GPU is unavailable.
**Thread Safety:** GPU dispatch is single-threaded per request; device-context ownership must not be shared across concurrent geo sessions.