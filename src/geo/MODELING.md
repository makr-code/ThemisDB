---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
Source_Level: "L1"
SOT_Domain: "module-behavior"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Geo data and query models

## Purpose and scope

This document describes how the geo module represents coordinates, geometries,
and spatial-query intermediates so developers can preserve their meaning when
adding an operation. It does not define a new storage schema or override the
[public contract](./API.md).

## Geometry representation

`themis::geo::GeometryInfo` in
[`ewkb.h`](../../include/utils/geo/ewkb.h) is the common parsed-geometry
container:

- `type` distinguishes the WKB geometry kinds and their Z variants.
- `srid` defaults to 4326; `has_z` and `has_m` describe dimensional flags.
  The header explicitly marks the M dimension as unsupported.
- `coords` holds simple geometry coordinates; `rings` represents polygon
  rings; `geometries` holds collection members.
- `computeMBR()` supplies a 2D minimum bounding rectangle; the `MBR` type can
  also carry optional Z bounds. Its `area()` is in square coordinate degrees,
  not square metres.
- `Coordinate` stores `x`, `y`, and optional `z`. GeoJSON and most geo math
  entry points interpret `x` as longitude and `y` as latitude, but callers
  must check each API signature for parameter order.

`EWKBParser` exposes EWKB, WKT, and GeoJSON parsing/serialization plus EWKB
validation. `GeoJSONGeometry` provides a separate polymorphic model with
`type`, `bbox`, `crs`, `toGeoJSON`, and `validate` operations; do not assume
that constructing either representation has already run every validator.

## Coordinate reference systems and validation

`CrsId` names WGS84 (EPSG:4326), Web Mercator (EPSG:3857), geocentric
coordinates (EPSG:4978), and `Custom`. Naming a CRS is not a coordinate
transformation: these geometry types do not expose a reprojection operation.
Do not pass coordinates from different CRSs into a distance, MBR, or backend
operation without an explicit conversion in the caller's data path.

The GeoJSON contract specifies `[longitude, latitude]`, finite WGS84 values,
closed polygon rings, at least two LineString positions, and at least four
positions in a polygon ring including the closing position. The
`GeoJSONGeometry` implementation also checks ring winding (exterior CCW,
interior CW). These are different from generic `GeometryInfo` storage and
from the separate `GeoValidator` helper; see
[`TRUST.md`](./TRUST.md) for the distinct size limits and enforcement caveat.

## Query and index models

- `MBR` intersection and containment are axis-aligned 2D comparisons.
  `GeoRTree` and cursor-backed index access use bounding boxes to locate keys;
  exact geometry predicates, where requested, are a refinement stage.
- `SpatialJoinPair` carries the two feature keys and a distance in metres.
  `SpatialJoinConfig::max_pairs` defaults to one million; an empty input side
  has no matching pairs under the frozen contract.
- `TemporalSpatialQuery` reads a geometry field (default name `location`) from
  a versioned document. Its bounding-box and distance helpers select using
  geometry centroids, not full-shape intersection; distance uses Haversine.
- `RasterGrid` is a row/column grid with geographic bounds, cell dimensions,
  float values, and a no-data marker. Tile coordinates use XYZ/Web Mercator
  conventions, independently of the raster grid's WGS84 bounds.
- [`EMBEDDING.md`](./EMBEDDING.md) describes the separate 3D unit-sphere
  projection used by geographic k-NN.

## Modeling constraints for changes

1. Preserve the API's coordinate order, CRS, and units explicitly in names or
   documentation; never infer these from `double` alone.
2. Keep candidate filtering distinct from exact predicates. A bounding-box
   result may include geometries that do not satisfy the final predicate.
3. Preserve Z only for operations that explicitly support it. The M measure
   dimension is not supported by the common EWKB geometry model.
4. Distinguish an invalid/missing temporal geometry (represented by an empty
   optional) from a valid geometry that simply does not match a query.
5. Do not treat arithmetic longitude/latitude averages as a global centroid
   model; the clustering API documents that approximation for local clusters.

## Model validation and measurements

Validate model semantics with fixed geometries and expected results, not only
with successful parsing:

- In `test_geo_json_geometry.cpp`, check GeoJSON serialization/parsing,
  coordinate preservation, and bbox behavior for each supported geometry
  shape.
- In `test_spatial_index.cpp`, compare MBR candidate results with expected
  overlap cases; in the spatial join suite, separately verify exact predicate
  results after candidate selection.
- In temporal-query fixtures, include a non-point geometry whose centroid is
  inside the query bounds and one whose bounding box overlaps while its
  centroid does not. This makes the documented centroid semantics explicit.
- Record coordinate round-trip delta in degrees, candidate-to-exact-result
  ratio for bbox prefilters, and distance delta against the selected reference
  method. These are evaluation measurements, not claimed current values.
- For latency, use the mapped cases in
  [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) and compare
  release-profile runs with a baseline; the values in that file are targets,
  not proof of a measured pass.

Keep the CRS, coordinate axis order, geometry extent, and backend fixed across
comparisons. The exact test target commands are described in
[`IMPLEMENTATION.md`](./IMPLEMENTATION.md).

## Sourcecode verification

- Geometry and bounds: [`ewkb.h`](../../include/utils/geo/ewkb.h),
  [`geo_json_geometry.h`](../../include/geo/geo_json_geometry.h),
  [`geo_json_geometry.cpp`](./geo_json_geometry.cpp).
- Query shapes: [`spatial_join.h`](../../include/geo/spatial_join.h),
  [`temporal_spatial_query.h`](../../include/geo/temporal_spatial_query.h),
  [`raster.h`](../../include/geo/raster.h), and
  [`tile_server.h`](../../include/geo/tile_server.h).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
