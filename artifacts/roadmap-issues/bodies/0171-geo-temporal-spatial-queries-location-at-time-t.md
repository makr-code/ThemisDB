### Context

This issue implements the roadmap item 'Temporal-Spatial Queries (Location at Time T)' for the geo domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Temporal-Spatial Queries (Location at Time T)

### Goal

Deliver the scoped changes for Temporal-Spatial Queries (Location at Time T) in src/geo/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Temporal-Spatial Queries (Location at Time T)
**Priority:** Medium
**Target Version:** v2.4.0
**Status:** ✅ Implemented in `include/geo/temporal_spatial_query.h` + `src/geo/temporal_spatial_query.cpp`

Bridge between the temporal versioning system (`SystemVersionedTable`) and geospatial queries.
Answers questions of the form "where was entity X at time T?" or "which entities were inside
region R at time T?".

**What was implemented:**
- `TemporalSpatialQuery::extractGeometry(doc, field)` — parse a GeoJSON geometry from a
  named field of a `VersionedDocument`; field value may be a JSON string or an embedded JSON
  object.
- `TemporalSpatialQuery::locationAtTime(table, key, as_of)` — return the geometry stored in
  the version of `key` that was current at `as_of` (ms since epoch); returns `std::nullopt`
  when the key did not exist at that time or the geometry field is absent / invalid.
- `TemporalSpatialQuery::allLocationsAtTime(table, as_of)` — return `(key, geometry)` pairs
  for every entity alive at time T that carries a parseable geometry field.
- `TemporalSpatialQuery::entitiesInBBoxAtTime(table, bbox, as_of)` — filter alive entities
  at time T whose geometry centroid falls inside the given axis-aligned bounding box (WGS-84).
- `TemporalSpatialQuery::entitiesWithinDistanceAtTime(table, lon, lat, distance_m, as_of)` —
  filter alive entities at time T within `distance_m` metres of a centre point; uses the
  Haversine spherical-earth formula via `haversineDistanceM()` from `spatial_join.h`.
- `TemporalSpatialQuery::entitiesWithinDistanceAtTimeSorted(...)` — same as above but returns
  `(document, distance_m)` pairs sorted ascending by distance.

**Implementation Notes:**
- All methods are `static` and stateless; thread-safe as long as the `SystemVersionedTable`
  reference remains stable during the call.
- Geometry is always read from the named field in `VersionedDocument::data` (default:
  `"location"`); a custom field name can be supplied via the `geo_field` parameter.
- Invalid or missing geometry fields produce `std::nullopt` / skip that row (no exception
  thrown to the caller); parse failures are logged at WARN level via `THEMIS_WARN`.
- The centroid representative point is used for BBox and distance filters: Point geometries
  use their single coordinate directly; all other types use `GeometryInfo::computeCentroid()`.

**Performance Targets:**
- `locationAtTime` on a table with 100 K rows: ≤ 1 ms (delegates to `SystemVersionedTable::getAsOf` which is O(log n)).
- `entitiesWithinDistanceAtTime` over 10 K alive entities: ≤ 50 ms single-threaded (linear scan; R-tree optimisation deferred to a future release).

---

### Acceptance Criteria

- [ ] `TemporalSpatialQuery::extractGeometry(doc, field)` — parse a GeoJSON geometry from a
- [ ] `TemporalSpatialQuery::locationAtTime(table, key, as_of)` — return the geometry stored in
- [ ] `TemporalSpatialQuery::allLocationsAtTime(table, as_of)` — return `(key, geometry)` pairs
- [ ] `TemporalSpatialQuery::entitiesInBBoxAtTime(table, bbox, as_of)` — filter alive entities
- [ ] `TemporalSpatialQuery::entitiesWithinDistanceAtTime(table, lon, lat, distance_m, as_of)` —
- [ ] `TemporalSpatialQuery::entitiesWithinDistanceAtTimeSorted(...)` — same as above but returns
- [ ] All methods are `static` and stateless; thread-safe as long as the `SystemVersionedTable`
- [ ] Geometry is always read from the named field in `VersionedDocument::data` (default:
- [ ] Invalid or missing geometry fields produce `std::nullopt` / skip that row (no exception
- [ ] The centroid representative point is used for BBox and distance filters: Point geometries
- [ ] `locationAtTime` on a table with 100 K rows: ≤ 1 ms (delegates to `SystemVersionedTable::getAsOf` which is O(log n)).
- [ ] `entitiesWithinDistanceAtTime` over 10 K alive entities: ≤ 50 ms single-threaded (linear scan; R-tree optimisation deferred to a future release).

### Relationships

- Roadmap row: #171 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/geo/FUTURE_ENHANCEMENTS.md#temporal-spatial-queries-location-at-time-t
- Source key: roadmap:171:geo:v1.7.0:temporal-spatial-queries-location-at-time-t

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:171:geo:v1.7.0:temporal-spatial-queries-location-at-time-t -->
<!-- roadmap-ref: row=171;module=geo;target=v1.7.0 -->
<!-- roadmap-detail: src/geo/FUTURE_ENHANCEMENTS.md#temporal-spatial-queries-location-at-time-t -->
