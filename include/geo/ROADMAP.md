> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/geo/ROADMAP.md -->

# GEO Module — Public Header Roadmap

**Module Path:** `include/geo/`
**Canonical implementation roadmap:** [`../../src/geo/ROADMAP.md`](../../src/geo/ROADMAP.md)

---

## Overview

This document tracks public API contract stability, planned header additions, and header-level breaking changes for `include/geo/`. For feature roadmap items that affect both implementation and headers see the canonical roadmap:

→ [`../../src/geo/ROADMAP.md`](../../src/geo/ROADMAP.md)

---

## Current Status

production geo runtime with CPU/GPU spatial backends, R-tree indexing, GeoJSON geometry, spatial joins, and raster support. All production-required public headers are present and `#pragma once` guarded.

The header API surface is **stable** for all types introduced in v1.x.

---

## Completed ✅

- [x] `spatial_backend.h` — backend execution contract
- [x] `gpu_kernel_dispatcher.h` — backend execution contract
- [x] `device_detector.h` — backend execution contract
- [x] `geo_rtree.h` — spatial index and geometry contract
- [x] `rtree_cursor.h` — spatial index and geometry contract
- [x] `geo_json_geometry.h` — spatial index and geometry contract
- [x] `geo_math.h` — spatial index and geometry contract
- [x] `geo_ops_ext.h` — spatial index and geometry contract
- [x] `spatial_join.h` — query and analytics contract
- [x] `spatial_join_filter.h` — query and analytics contract
- [x] `geo_clustering.h` — query and analytics contract
- [x] `geo_faiss_knn.h` — query and analytics contract
- [x] `raster.h` — query and analytics contract
- [x] `raster_query_interface.h` — query and analytics contract
- [x] `temporal_spatial_query.h` — query and analytics contract
- [x] `temporal_spatial_query_builder.h` — query and analytics contract

---

## In Progress 🚧

- [I] Header-level unit test coverage for all public interfaces (tracked via module issue backlog)

---

## Planned Features 📋

### Short-term (Next 3–6 months)

- [ ] Audit all headers for missing `[[nodiscard]]` on factory and error-returning methods (Target: Q3 2026)
- [ ] Verify `#pragma once` guard consistency across all headers in a CI step (Target: Q3 2026)

### Medium-term (6–12 months)

- [ ] Align header-level type documentation with OpenAPI spec where applicable (Target: Q4 2026)
- [ ] Consolidate deprecated symbol annotations with `[[deprecated("...")]]` where needed (Target: Q4 2026)

---

## Production Readiness Checklist

- [x] All headers have `#pragma once` guard
- [x] All public factory methods marked `[[nodiscard]]`
- [x] Build conditionals documented in `README.md` and `ARCHITECTURE.md`
- [P] Header-level unit tests (tracked in module issue backlog)

---

## References

- Canonical implementation roadmap: [`../../src/geo/ROADMAP.md`](../../src/geo/ROADMAP.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Module overview: [`README.md`](README.md)
