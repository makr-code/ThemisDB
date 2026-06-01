> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/geo/FUTURE_ENHANCEMENTS.md -->

# Geo Module — Public Header Future Enhancements

**Module Path:** `include/geo/`
**Canonical implementation enhancements:** [`../../src/geo/FUTURE_ENHANCEMENTS.md`](../../src/geo/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/geo/`. Runtime R-tree, FAISS, raster, and GPU-dispatch work remain tracked in:

→ [`../../src/geo/FUTURE_ENHANCEMENTS.md`](../../src/geo/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Geometry headers must follow GeoJSON RFC 7946 coordinate model; internal precision must not be exposed.
- `[x]` R-tree headers must define stable index-access contracts; node split strategies stay internal.
- `[x]` GPU acceleration headers must be conditional on device availability via `DeviceDetector`.
- `[x]` Temporal-spatial headers must integrate with `include/temporal/` period semantics.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `GeoRTree` insert / range-query | `geo_rtree.h` | Spatial query engine | ✅ Stable |
| `GeoFAISSKNN` nearest-neighbour | `geo_faiss_knn.h` | ML-driven geo search | ✅ Stable |
| `SpatialJoin` join API | `spatial_join.h` | Analytical query layer | ✅ Stable |
| `TileServer` serve API | `tile_server.h` | Map front-ends and tile consumers | ✅ Stable |
| `TemporalSpatialQuery` execute | `temporal_spatial_query.h` | Temporal-GIS workflows | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Document GPU-fallback behaviour (scalar / CPU path) consistently across FAISS KNN and GPU kernel headers.
- Align temporal-spatial query builder period types with `include/temporal/temporal_types.h` aliases.
- Add RFC 7946 conformance annotations to GeoJSON geometry headers.

### Medium-Term (Q4 2026)

- Introduce `geo_policy.h` to provide per-query spatial resource quotas and access-policy contract.
- Expose benchmark-reference latency targets for KNN, R-tree range, and spatial-join hot paths.
- Add deprecation guidance for legacy coordinate-system assumptions and document CRS-aware migration paths.

### Long-Term

- Unify spatial and temporal-spatial result types behind a shared geo-context envelope for analytical consumers.
- Add extension hooks for embedders to inject alternative spatial index backends via `ISpatialBackend`.
- Provide spatial-query explain plans via `geo_rtree.h` / `spatial_join.h` to surface index-pruning decisions.
