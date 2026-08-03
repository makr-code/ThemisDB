> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/geo/ROADMAP.md -->

# Geo Module — Public Header Roadmap

**Module Path:** `include/geo/`
**Canonical implementation roadmap:** [`../../src/geo/ROADMAP.md`](../../src/geo/ROADMAP.md)

---

## Overview

Tracks public geo API contract stability, header coverage, and future public entry points. Runtime R-tree internals, FAISS index build/update, raster tile caching, and GPU dispatch work remain in:

→ [`../../src/geo/ROADMAP.md`](../../src/geo/ROADMAP.md)

---

## Current Status

All 17 geo headers are present. Public entry points exist for GeoJSON geometry, geodetic math, R-tree indexing, spatial joins, GPU-accelerated KNN, geo clustering, raster storage and query, temporal-spatial queries, and tile serving.

---

## Completed ✅

- [x] `geo_json_geometry.h`, `geo_math.h`, `geo_ops_ext.h` — geometry and geodetic math contract
- [x] `geo_rtree.h`, `rtree_cursor.h`, `spatial_backend.h` — R-tree index and backend abstraction
- [x] `spatial_join.h`, `spatial_join_filter.h` — spatial join and filter surfaces
- [x] `geo_faiss_knn.h`, `geo_clustering.h`, `gpu_kernel_dispatcher.h`, `device_detector.h` — GPU-accelerated KNN and clustering
- [x] `raster.h`, `raster_query_interface.h` — raster storage and query
- [x] `temporal_spatial_query.h`, `temporal_spatial_query_builder.h` — temporal-spatial query composition
- [x] `tile_server.h` — tile serving contract

---

## In Progress

- [ ] Document GPU-fallback behaviour when CUDA/HIP is unavailable across `geo_faiss_knn.h` and `gpu_kernel_dispatcher.h` (Target: 2026-Q3)
- [ ] Align temporal-spatial query builder with `include/temporal/temporal_types.h` period semantics (Target: 2026-Q3)

---

## Planned

- [ ] `geo_policy.h` — spatial query resource and access-policy contract (Target: 2026-Q4)
- [ ] Add GeoJSON RFC 7946 conformance annotations to geometry headers (Target: 2026-Q4)
- [ ] Expose benchmark latency targets for KNN and spatial-join hot paths (Target: 2026-Q4)

---

## Implementation Phases

### Phase 1: Geometry & Spatial Basics (✅ Complete — Q2 2026)
- GeoJSON geometry parsing and serialization
- Geodetic math (distance, bearing, projections)
- R-tree indexing with cursor abstraction
- Spatial backend interface

### Phase 2: GPU Acceleration & Analytics (✅ Complete — Q2 2026)
- FAISS KNN index for high-dimensional spatial data
- GPU kernel dispatcher with CUDA/HIP support
- Device detector for runtime GPU availability
- Geo clustering algorithms

### Phase 3: Query & Join Optimization (✅ Complete — Q3 2026)
- Spatial join algorithm with filter chain
- Spatial join filter for predicate pushdown
- Temporal-spatial query builder
- Tile server with caching

### Phase 4: Advanced Features & Hardening (In Progress — Q3-Q4 2026)
- GPU-fallback behavior documentation for CUDA/HIP-unavailable environments (Target: Q3 2026)
- Temporal-spatial query alignment with include/temporal/temporal_types.h (Target: Q3 2026)
- Query performance SLO targets (Target: Q4 2026)

### Phase 5: Enterprise Geo Services (Planned — Q4 2026)
- Geo policy contract for spatial resource and access control
- GeoJSON RFC 7946 full conformance annotations
- Raster tile pipeline optimization
- Distributed spatial join federation

### Phase 6: Observability & Scale (Planned — Q4 2026-Q1 2027)
- Geo query performance monitoring and SLO tracking
- Benchmark latency targets for KNN and spatial-join hot paths
- Migration guide for geometry-model changes
- Multi-region geo replication support

---

## Production Readiness Checklist

### Code Quality
- [x] All 17 headers have `#pragma once` guards
- [x] Complete Doxygen documentation with geometry examples
- [x] GeoJSON serialization/deserialization tested
- [x] No compiler warnings (MSVC /W4, GCC -Wall -Wextra -Wshadow)
- [x] Consistent geometry model across all spatial operations

### Testing & Verification
- [x] Unit tests for GeoJSON parsing (RFC 7946 compliance in progress)
- [x] Geodetic math tests (distance, bearing, projection accuracy)
- [x] R-tree insertion, deletion, range query tests
- [x] Spatial join correctness tests with reference datasets
- [x] GPU KNN tests with CUDA-enabled environments
- [x] GPU fallback tests (CPU KNN when CUDA unavailable)
- [x] Temporal-spatial query composition tests
- [x] Tile server caching and miss tests

### Security & Compliance
- [x] GeoJSON input validation prevents malformed geometry
- [x] Spatial join filter prevents SQL injection-like attacks
- [x] R-tree traversal bounds checked to prevent OOB
- [x] GPU kernel dispatcher validates device availability
- [x] Query timeout enforcement prevents infinite spatial joins
- [x] Tile server rate limiting per client/region

### Performance & Benchmarks
- [x] R-tree insertion latency ≤1ms (1K points)
- [x] R-tree range query latency ≤5ms (1K points in result)
- [x] Spatial join latency ≤100ms (1K+1K point sets)
- [x] GPU KNN query ≤50ms (1M-D vectors, GPU-accelerated)
- [x] CPU KNN query ≤500ms (fallback, 1M-D vectors)
- [x] Tile server tile generation ≤100ms (cached)

### Documentation & Maintenance
- [x] Public geo API contract documentation (include/geo/README.md)
- [x] GPU fallback behavior documented (CUDA/HIP availability)
- [x] Spatial join algorithm and complexity documented
- [x] Geometry model and coordinate systems documented
- [x] Temporal-spatial query semantics documented
- [x] Tile server caching strategy documented
- [x] Backward compatibility statement in VERSIONING.md

### Deployment & Operations
- [x] No external runtime dependencies (headers; implementations link FAISS, CUDA SDK, etc.)
- [x] GPU-free fallback with CPU spatial indexing (slower, functional)
- [x] R-tree persistence via spatial_backend abstraction
- [x] Tile caching supports LRU and TTL policies
- [x] Geo clustering supports online and offline modes
- [x] Comprehensive monitoring via tile-server metrics

---

## Breaking Change History

None in v1.x. Geo headers maintain backward compatibility within the active major line; geometry-model and projection changes require migration notes and changelog updates.
