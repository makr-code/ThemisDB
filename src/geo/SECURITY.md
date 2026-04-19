> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Geo Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Geo module provides geospatial query operations (contains, intersects, distance, ST_BUFFER, ST_UNION, ST_DIFFERENCE, clustering, raster queries, and temporal-spatial queries). Security concerns include: validation of geometry inputs to prevent parser attacks, safe GPU/CPU fallback behavior, and protecting against malformed GeoJSON.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Malformed GeoJSON causing parser crash | Full RFC 7946 GeoJSON parser validates all 7 geometry types; null coordinates and non-finite values rejected before processing |
| Self-intersecting polygon causing infinite loop | `ST_BUFFER` and containment checks use Boost.Geometry which handles degenerate polygons; GPU kernels skip invalid geometries |
| GPU kernel launch with invalid geometry | `GeoAccelerationBridge` validates geometry before dispatch; NaN/Inf coordinates rejected at bridge layer |
| Resource exhaustion via enormous polygon | Polygon vertex count limits enforced before CUDA kernel launch; bounding box pre-check skips distant geometries |
| GPU fallback audit trail manipulation | `DeviceDetector` and GPU backend switches are logged to the structured audit log; log entries cannot be suppressed by callers |
| CUDA kernel buffer overflow | Kernel inputs (point count, polygon vertex count) are validated against max batch size before kernel launch |
| Tile server SSRF via tile URL parameters | Tile server validates tile coordinates (x, y, z) as integers within valid range; no external URL fetching |
| Temporal-spatial query with invalid timestamps | Timestamp values validated as non-negative integers; `NaN`/`Inf` timestamps are rejected |
| Cross-tenant geospatial data access | Geo queries are always scoped to a tenant's collections; no shared geometry indices across tenants |

## Security Controls

### Geometry Input Validation
- All geometry inputs are validated for NaN/Inf coordinates before any backend dispatch.
- GeoJSON RFC 7946 parser rejects geometries with out-of-range coordinates (latitude > 90°, longitude > 180°).
- `IGeoJSONGeometry::validate()` returns a typed `ValidationResult` with per-error `ValidationError` entries; out-of-range coordinates and invalid ring winding order are reported without silent clamping (`include/geo/geo_json_geometry.h`).
- Self-intersecting polygon detection is available on Boost.Geometry backend; GPU backend relies on pre-validation.
- Polygon vertex counts are capped before CUDA kernel launch to prevent buffer overflow.
- `DWithinFilter` (in `include/geo/spatial_join_filter.h`) computes distances with the Haversine formula; NaN/Inf coordinates are rejected before the predicate is evaluated.

### GPU/CPU Backend Safety
- Circuit-breaker pattern: GPU backend automatically falls back to CPU when no GPU device is present or when GPU errors occur.
- All GPU↔CPU backend switches are recorded in the structured audit log.
- `DeviceDetector` wraps `themis::gpu::DeviceDiscovery` with geo-specific VRAM and compute-capability checks.
- `ST_BUFFER` on the GPU backend always delegates to CPU with an audit log entry — GPU ST_BUFFER kernels are deferred.

### Tile Server
- Tile coordinates (x, y, zoom) are validated as bounded integers before raster tile retrieval.
- No external tile sources are fetched; only locally computed vector tiles are served.

## Data Handling

- Geo module processes geometry coordinates (WGS84 lat/lon); no PII is associated with geometry data by default.
- Location history for temporal-spatial queries may constitute personal location data — subject to GDPR if associated with individuals; handled at the collection schema level.
- Raster data (elevation, heatmaps) is processed from locally stored raster files; no external data fetching.
- GPU device memory holds intermediate geometry computation results; not persisted after query completion.

## Known Limitations

- `ST_BUFFER`, `ST_UNION`, and `ST_DIFFERENCE` CUDA kernels are not yet implemented; GPU backend delegates these to CPU (with audit log).
- WGS-84 ellipsoidal geometry (Issue #1744) is not yet implemented; all distance calculations use the Haversine spherical approximation.
- R-tree index is not persisted across restarts; rebuilt on first query.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| Boost.Geometry | CPU-exact geospatial operations | Keep patched |
| S2 Geometry Library | S2 cell indexing | Keep patched |
| H3 (Uber) | Hexagonal grid indexing | Keep patched |
| CUDA Toolkit (optional) | GPU kernel dispatch | Version-pinned via acceleration module |
| ROCm/HIP (optional) | AMD GPU backend | Keep patched |
