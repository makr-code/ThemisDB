### Context

This issue implements the roadmap item 'Spatial JOIN Support' for the geo domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Spatial JOIN Support

### Goal

Deliver the scoped changes for Spatial JOIN Support in src/geo/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Spatial JOIN Support
**Priority:** Low
**Target Version:** v2.2.0
**Status:** ✅ Implemented in `include/geo/spatial_join.h` + `src/geo/spatial_join.cpp`

Add a spatial JOIN operation that finds all pairs (A, B) from two geometry collections where `distance(A, B) ≤ threshold`. This enables nearest-neighbour and within-radius multi-collection queries from AQL.

**Implementation Notes:**
- Implement as a nested-loop join with R-tree index on the inner collection (`SpatialIndex::rTreeQuery(expandedBbox)`); avoids O(n²) brute-force for typical cardinalities.
- Add AQL syntax `FOR a IN colA FOR b IN colB FILTER GEO_DISTANCE(a.loc, b.loc) <= 1000 RETURN ...` via a new join rule in the AQL query optimizer.
- Expose join result as a lazy iterator to avoid materializing all pairs in memory; yield one pair at a time to the AQL execution engine.
- Add a configurable `max_pairs` limit (default 1 M) with a warning log when the limit is hit to prevent runaway queries.

**Performance Targets:**
- Spatial JOIN of two 100 000-point collections with 1 km threshold returns first 1 000 results in ≤ 500 ms.
- Memory usage during JOIN bounded by R-tree index size + O(batch_size) working set.

---

### Acceptance Criteria

- [ ] Implement as a nested-loop join with R-tree index on the inner collection (`SpatialIndex::rTreeQuery(expandedBbox)`); avoids O(n²) brute-force for typical cardinalities.
- [ ] Add AQL syntax `FOR a IN colA FOR b IN colB FILTER GEO_DISTANCE(a.loc, b.loc) <= 1000 RETURN ...` via a new join rule in the AQL query optimizer.
- [ ] Expose join result as a lazy iterator to avoid materializing all pairs in memory; yield one pair at a time to the AQL execution engine.
- [ ] Add a configurable `max_pairs` limit (default 1 M) with a warning log when the limit is hit to prevent runaway queries.
- [ ] Spatial JOIN of two 100 000-point collections with 1 km threshold returns first 1 000 results in ≤ 500 ms.
- [ ] Memory usage during JOIN bounded by R-tree index size + O(batch_size) working set.

### Relationships

- Roadmap row: #170 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/geo/FUTURE_ENHANCEMENTS.md#spatial-join-support
- Source key: roadmap:170:geo:v1.6.0:spatial-join-support

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:170:geo:v1.6.0:spatial-join-support -->
<!-- roadmap-ref: row=170;module=geo;target=v1.6.0 -->
<!-- roadmap-detail: src/geo/FUTURE_ENHANCEMENTS.md#spatial-join-support -->
