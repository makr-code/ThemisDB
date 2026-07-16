### Context

This issue implements the roadmap item 'R-tree Spatial Index for CPU Backend' for the geo domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.5.0.

Primary detail section: R-tree Spatial Index for CPU Backend

### Goal

Deliver the scoped changes for R-tree Spatial Index for CPU Backend in src/geo/ and complete the linked detail section in a release-ready state for v1.5.0.

### Detailed Scope

### R-tree Spatial Index for CPU Backend
**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented in `src/geo/geo_rtree.cpp` + `include/geo/geo_rtree.h`

Replace the linear scan in `boost_cpu_exact_backend.cpp` for `contains` and `intersects` queries with an in-memory R-tree index (Boost.Geometry `rtree` with `rstar` packing algorithm). For collections with > 10 000 geometries the current O(n) scan becomes the dominant query cost.

**Implementation Notes:**
- Add `SpatialIndex` class in a new `spatial_index.cpp`; wrap `boost::geometry::index::rtree<value, bgi::rstar<16>>`.
- Index is built lazily on first spatial query and cached per collection handle; invalidated on write.
- Expose `SpatialIndex::bulkLoad(geometries)` for cold-start performance (STR packing is 3–5× faster than incremental insert for read-heavy workloads).
- Index memory usage must be reported via the existing structured audit log (`geo_index_bytes_allocated` field) so operators can observe RSS growth.

**Performance Targets:**
- `intersects` query over 1 M point geometries: ≤ 5 ms p99 with R-tree vs ~2 s with linear scan.
- Bulk-load of 1 M geometries into the R-tree ≤ 3 s wall clock.

**Scientific References:**
- [1] Guttman, A. (1984). R-Trees: A Dynamic Index Structure for Spatial Searching. *Proceedings of the 1984 ACM SIGMOD International Conference on Management of Data*, 47–57. https://doi.org/10.1145/602259.602266
- [2] Beckmann, N., Kriegel, H.-P., Schneider, R., & Seeger, B. (1990). The R*-Tree: An Efficient and Robust Access Method for Points and Rectangles. *Proceedings of the 1990 ACM SIGMOD International Conference on Management of Data*, 322–331. https://doi.org/10.1145/93597.98741
- [3] Leutenegger, S. T., Lopez, M. A., & Edgington, J. (1997). STR: A Simple and Efficient Algorithm for R-Tree Packing. *Proceedings of the 13th IEEE International Conference on Data Engineering*, 497–506. https://doi.org/10.1109/ICDE.1997.582015

---

### Acceptance Criteria

- [ ] Add `SpatialIndex` class in a new `spatial_index.cpp`; wrap `boost::geometry::index::rtree<value, bgi::rstar<16>>`.
- [ ] Index is built lazily on first spatial query and cached per collection handle; invalidated on write.
- [ ] Expose `SpatialIndex::bulkLoad(geometries)` for cold-start performance (STR packing is 3–5× faster than incremental insert for read-heavy workloads).
- [ ] Index memory usage must be reported via the existing structured audit log (`geo_index_bytes_allocated` field) so operators can observe RSS growth.
- [ ] `intersects` query over 1 M point geometries: ≤ 5 ms p99 with R-tree vs ~2 s with linear scan.
- [ ] Bulk-load of 1 M geometries into the R-tree ≤ 3 s wall clock.
- [ ] [1] Guttman, A. (1984). R-Trees: A Dynamic Index Structure for Spatial Searching. *Proceedings of the 1984 ACM SIGMOD International Conference on Management of Data*, 47–57. https://doi.org/10.1145/602259.602266
- [ ] [2] Beckmann, N., Kriegel, H.-P., Schneider, R., & Seeger, B. (1990). The R*-Tree: An Efficient and Robust Access Method for Points and Rectangles. *Proceedings of the 1990 ACM SIGMOD International Conference on Management of Data*, 322–331. https://doi.org/10.1145/93597.98741
- [ ] [3] Leutenegger, S. T., Lopez, M. A., & Edgington, J. (1997). STR: A Simple and Efficient Algorithm for R-Tree Packing. *Proceedings of the 13th IEEE International Conference on Data Engineering*, 497–506. https://doi.org/10.1109/ICDE.1997.582015

### Relationships

- Roadmap row: #67 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/geo/FUTURE_ENHANCEMENTS.md#r-tree-spatial-index-for-cpu-backend
- Source key: roadmap:67:geo:v1.5.0:r-tree-spatial-index-for-cpu-backend

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:67:geo:v1.5.0:r-tree-spatial-index-for-cpu-backend -->
<!-- roadmap-ref: row=67;module=geo;target=v1.5.0 -->
<!-- roadmap-detail: src/geo/FUTURE_ENHANCEMENTS.md#r-tree-spatial-index-for-cpu-backend -->
