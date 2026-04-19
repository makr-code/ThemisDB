# Geo Module - Future Header Enhancements

## Scope

- `IGeoIndex` interface extensions for spatial index backend abstraction
- GeoJSON geometry type API covering all RFC 7946 geometry types as immutable value types
- Spatial JOIN filter interface for composable geospatial query predicates
- R-tree cursor API for range and nearest-neighbor traversal
- Raster data query interface (compile-time optional via feature flag)
- Temporal-spatial query builder API for time-windowed geospatial queries

## Design Constraints

- [x] GeoJSON geometry types are immutable value types; mutation returns a new instance
- [x] Spatial JOIN is expressed as a composable `ISpatialJoinFilter` rather than a monolithic query type
- [ ] GPU dispatch is compile-time optional via `THEMIS_ENABLE_GPU`; all interfaces must compile without it
- [ ] Raster interface is compile-time optional via `THEMIS_ENABLE_RASTER`; absent symbols produce a clear `static_assert`
- [ ] R-tree cursor is pull-based and single-threaded per cursor instance; parallel traversal uses multiple cursors
- [x] Coordinate reference system (CRS) is a required constructor argument for all geometry types; no implicit WGS84

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IGeoIndex` | Spatial query engine, collection scan, JOIN planner | Abstract spatial index; concrete impls: R-tree, flat scan, GPU-backed |
| `IGeoJSONGeometry` | GeoJSON serializer, spatial predicate evaluator, REST API | Base for Point, LineString, Polygon, MultiPolygon, GeometryCollection |
| `ISpatialJoinFilter` | Query planner, JOIN executor, filter pipeline | Composable; supports `AND`, `OR`, `NOT` combinators |
| `IRTreeCursor` | Range query, k-NN query, index scan operator | Pull-based; `next()` returns next matching entry or `END` |
| `IRasterQueryInterface` | Raster tile server, spatial analytics, heatmap generator | Compile-time optional; guarded by `THEMIS_ENABLE_RASTER` |
| `ITemporalSpatialQueryBuilder` | Time-series geo queries, movement analytics | Fluent builder; immutable once `build()` is called |

## Planned Features

### Full GeoJSON Geometry Type API

- [x] Define `IGeoJSONGeometry` base with `type()`, `bbox()`, `crs()`, and `toGeoJSON()` methods
- [x] Concrete types: `GeoPoint`, `GeoLineString`, `GeoPolygon`, `GeoMultiPolygon`, `GeoGeometryCollection`
- [x] All types expose `validate()` returning a `ValidationResult` with detailed error codes
- [x] `GeoPolygon` enforces right-hand rule winding order on construction; returns error on violation
- [ ] Geometry equality uses coordinate tolerance configurable via `GeoConfig::coordinateTolerance()`

### Spatial JOIN Filter Interface

- [x] Define `ISpatialJoinFilter` with `matches(const IGeoJSONGeometry&, const IGeoJSONGeometry&) -> bool`
- [x] Built-in predicates: `Intersects`, `Contains`, `Within`, `Touches`, `DWithin(radius)`
- [x] Filters composable via `SpatialJoinFilter::and_()`, `or_()`, `not_()` factory methods
- [x] Filter instances are immutable after construction; safe to share across threads

### R-tree Cursor API

- [ ] Define `IRTreeCursor` with `next(GeoIndexEntry&) -> CursorStatus`
- [ ] Cursor opened via `IGeoIndex::openRangeCursor(bbox)` and `IGeoIndex::openKNNCursor(point, k)`
- [ ] Cursor exposes `estimatedResultCount()` for query planning
- [ ] Cursor invalidated if the underlying index is mutated; `next()` returns `CursorStatus::STALE`

### Temporal-Spatial Query Builder

- [ ] Define `ITemporalSpatialQueryBuilder` with fluent `withinBBox()`, `duringInterval()`, `withPredicate()` methods
- [ ] `build()` returns an immutable `TemporalSpatialQuery` value type
- [ ] Query supports time-window types: `POINT_IN_TIME`, `INTERVAL`, `SLIDING_WINDOW`
- [ ] Builder validates that temporal and spatial constraints are both set before `build()` succeeds

### Raster Data Query Interface

- [ ] Define `IRasterQueryInterface` (guarded by `#ifdef THEMIS_ENABLE_RASTER`) with `queryTile(TileKey)` and `queryBBox(BBox, resolution)`
- [ ] Tile size bounded by `RasterConfig::maxTileSizeBytes()`
- [ ] Raster queries return `RasterResult` containing band data, resolution metadata, and CRS info
- [ ] No-op stub provided when `THEMIS_ENABLE_RASTER` is not defined, returning `RasterStatus::NOT_SUPPORTED`

## Test Strategy

- Unit-test each GeoJSON geometry type for RFC 7946 compliance: valid construction, invalid inputs, `validate()` error codes
- Test `ISpatialJoinFilter` combinators with truth-table coverage for `and_()`, `or_()`, `not_()` across all built-in predicates
- Integration-test `IRTreeCursor` range queries against a 1 M-point synthetic dataset; verify result set matches brute-force scan
- Test `ITemporalSpatialQueryBuilder` for all three time-window types; verify query immutability after `build()`
- Compile-flag test: build with and without `THEMIS_ENABLE_GPU` and `THEMIS_ENABLE_RASTER`; assert no linker errors
- Fuzz-test geometry `validate()` with random byte sequences to ensure no crashes or UB on malformed input

## Performance Targets

- Geometry containment check (`ISpatialJoinFilter::matches()`) ≤ 500 ns for polygon vs. point on a single thread
- R-tree range query (`IRTreeCursor` full traversal) ≤ 1 ms for 1 M indexed points with a selectivity of 0.1%
- Spatial JOIN dispatch (filter evaluation over 100 K pairs) ≤ 5 ms on a 4-core reference machine
- Temporal-spatial query (`ITemporalSpatialQueryBuilder::build()` + execution) ≤ 10 ms for 1 M records with a 1-hour window
- GeoJSON geometry construction and `validate()` ≤ 10 µs per geometry for simple polygons (≤ 100 vertices)
- `IGeoIndex::openRangeCursor()` setup ≤ 500 µs regardless of index size

## Security / Reliability

- Geometry inputs validated for NaN, infinity, and out-of-range coordinates in `IGeoJSONGeometry::validate()` before any index operation
- Raster queries bounded by `RasterConfig::maxTileSizeBytes()` to prevent memory exhaustion via oversized tile requests
- Spatial JOIN access controlled by collection-level ACL; `ISpatialJoinFilter` does not bypass collection permissions
- `IRTreeCursor` returns `CursorStatus::STALE` on concurrent index mutation rather than producing incorrect results
- CRS mismatch between geometries surfaces as `SpatialError::CRS_MISMATCH` rather than silent coordinate misinterpretation
- Temporal-spatial queries with unbounded time windows are rejected with `QueryError::UNBOUNDED_WINDOW` unless explicitly opted in
