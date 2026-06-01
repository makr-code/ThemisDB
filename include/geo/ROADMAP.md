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

## Breaking Change History

None in v1.x. Geo headers maintain backward compatibility within the active major line; geometry-model and projection changes require migration notes and changelog updates.
