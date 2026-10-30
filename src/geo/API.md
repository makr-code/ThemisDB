---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
Source_Level: "L1"
SOT_Domain: "api-contract"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Geo public API guide

## Purpose and contract authority

This page is a developer-facing map of the geo interfaces. The binding v1.x
contract, error codes, coordinate validation commitments, and compatibility
policy are in [`geo_api_contract.h`](../../include/geo/geo_api_contract.h).
This page explains how to find and use those interfaces; it does not create a
second contract. Breaking contract changes require the version/migration
handling described by that header.

## Interface map

| Header | Main surface | Use |
|---|---|---|
| [`geo_api_contract.h`](../../include/geo/geo_api_contract.h) | `GeoErrorCode` and contract constants | Shared validation, fallback, index, and join contract |
| [`utils/geo/ewkb.h`](../../include/utils/geo/ewkb.h) | `GeometryInfo`, `Coordinate`, `MBR`, `EWKBParser` | Parse/serialize EWKB, WKT, and GeoJSON; carry geometry data |
| [`geo_json_geometry.h`](../../include/geo/geo_json_geometry.h) | `IGeoJSONGeometry`, `GeoPoint`, `GeoLineString`, `GeoPolygon`, collections | Typed geometry validation and GeoJSON serialization |
| [`spatial_backend.h`](../../include/geo/spatial_backend.h) | `ISpatialComputeBackend`, `GeoPrecisionMode` | Exact/approximate compute backend interface and registry functions |
| [`geo_backend_dispatch.h`](../../include/geo/geo_backend_dispatch.h) | `GeoBackendDispatcher` | Batched Haversine, Vincenty, and point-in-polygon dispatch |
| [`geo_rtree.h`](../../include/geo/geo_rtree.h), [`rtree_cursor.h`](../../include/geo/rtree_cursor.h) | `GeoRTree`, cursor/index interface | Insert, bulk load, bbox/predicate lookup, and continuation |
| [`spatial_join.h`](../../include/geo/spatial_join.h) | `SpatialJoin`, `SpatialJoinIterator` | Key-pair join with threshold distance and pair cap |
| [`geo_clustering.h`](../../include/geo/geo_clustering.h) | `dbscanCluster`, `kmeansCluster` | Point clustering with CPU/GPU configuration |
| [`geo_policy.h`](../../include/geo/geo_policy.h) | `SpatialQueryPolicy` | Per-query resource declarations and advisory collection list |
| [`raster.h`](../../include/geo/raster.h), [`raster_query_interface.h`](../../include/geo/raster_query_interface.h) | `RasterGrid`, `IRasterQueryInterface` | Grid sampling, bbox/tile queries, heatmap generation |
| [`temporal_spatial_query.h`](../../include/geo/temporal_spatial_query.h) | `TemporalSpatialQuery` | Geometry lookup/filter at a system-versioned time |
| [`tile_server.h`](../../include/geo/tile_server.h) | XYZ/Web Mercator helpers and vector-tile encoding | Geographic tile-address conversion and feature encoding |
| [`geo_faiss_knn.h`](../../include/geo/geo_faiss_knn.h) | `GeoFaissKnn` | Point-only nearest-neighbor and radius search bridge |
| [`geo_operator_diagnostics.h`](../../include/geo/geo_operator_diagnostics.h) | `GeoOperatorDiagnostics` | In-process incident recording and bounded recent-history access |

## Caller-facing details

- GeoJSON coordinates are `[longitude, latitude]`. Other interfaces explicitly
  name their fields/arguments; for example, `GeoBackendDispatcher::Point`
  stores `lat_deg` then `lon_deg`, while `haversineDistanceM` takes
  `(lon1, lat1, lon2, lat2)`.
- Units vary by entry point: `haversineDistanceM`, join results, and k-NN
  results use metres; `GeoBackendDispatcher` batch distances are documented in
  kilometres. Check each declaration before passing or comparing values.
- `GeoErrorCode` distinguishes geometry validation, backend, index, and
  join/query failures. `GPU_FALLBACK_TO_CPU` is informational; `BACKEND_UNAVAILABLE`
  describes the case where both paths are unavailable under the contract.
- `SpatialQueryPolicy::allowed_collections` is advisory; authoritative access
  checks belong at the server boundary. An empty list means no collection
  restriction in that policy object.
- The frozen contract declares constants including 10,000,000 coordinates
  per geometry, 100,000 query results before cursor continuation, and
  10,000,000 join inputs per side. These are contract declarations, not a
  statement that one shared validator enforces every limit on every path.
  `GeoValidator` has its own 10 MiB/100,000-coordinate helper bounds; see
  [`TRUST.md`](./TRUST.md) before interpreting either as a deployment-wide
  enforcement guarantee.
- Optional features may return explicit unsupported statuses, such as raster
  queries when raster support is not compiled. GPU availability is also
  conditional; see [`IMPLEMENTATION.md`](./IMPLEMENTATION.md).

## API change validation

For a contract change, update the header and its focused contract tests, then
run the relevant production implementation test and backend-specific test.
`test_geo_contract_hardening_focused.cpp` uses mocks, so it cannot alone prove
the concrete parser, index, query, or GPU behavior. See
[`IMPLEMENTATION.md`](./IMPLEMENTATION.md) for test registration and command
shape.

## Sourcecode verification

- Contract: [`geo_api_contract.h`](../../include/geo/geo_api_contract.h).
- Concrete declarations are linked in the interface map above.
- Server-side authorization boundary: [`spatial_api_handler.cpp`](../../src/server/spatial_api_handler.cpp)
  and [`geo_policy.h`](../../include/geo/geo_policy.h).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
