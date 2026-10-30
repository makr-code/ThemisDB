---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
Source_Level: "L1"
SOT_Domain: "operations"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Geo incident runbook

## Purpose and safety boundary

Use this runbook to diagnose failed or degraded geo requests. It covers the
geo library and its observable integration points; the owning server and
deployment determine endpoint access, process restart, and release rollback.
This page does not authorize clearing or rebuilding a live spatial index.

## First response

1. Record the affected operation, time window, build revision, query/backend
   configuration, CRS, coordinate order, geometry type, and returned
   `GeoErrorCode` or raster status. Do not copy raw customer geometry into an
   unrestricted incident ticket.
2. Capture the geo device report and GPU backend statistics using
   `getGeoDeviceReportJson()` and `getGpuSpatialBackendStatsJson()` from
   [`spatial_backend.h`](../../include/geo/spatial_backend.h). The server
   spatial handler also attaches GPU backend statistics to its response; use
   the owning server API's documented access controls.
3. If the integration exposes `GeoOperatorDiagnostics`, capture
   `recentIncidents()`, `incidentsBySeverity()`, `totalIncidentCount()`, and
   `formatSummary()` before clearing or restarting it. The in-memory history
   retains at most 256 recent incidents; it is not a durable incident store.
4. Preserve logs, backend/device reports, query parameters after redaction,
   and whether a CPU fallback occurred. `GeoOperatorDiagnostics` descriptions
   are retained as supplied, so keep secrets and raw geometry out of them.
5. Check the service owner's health and deployment state before changing
   backend flags or retrying load-bearing work.

## Symptom: GPU requests fall back to CPU or latency rises

1. Compare `THEMIS_GEO_CUDA` / `THEMIS_GEO_HIP` build selection with the
   deployed artifact. A compiled GPU path does not prove that the runtime
   device meets geo capability requirements.
2. Inspect `getGeoDeviceReportJson()` and backend statistics; distinguish a
   device that was not selected from a failed kernel or transfer.
3. Check fallback and capability-drift incidents, driver/runtime health, and
   GPU memory pressure with the platform owner. Preserve the report before
   restarting, since the incident registry is in memory.
4. If service availability requires a known CPU-only mode, reconfigure and
   rebuild with both geo GPU flags disabled, verify CMake reports the CPU
   dispatcher path, and deploy using the service's normal rollback/change
   procedure. Do not change flags as a live runtime toggle.
5. Re-enable a GPU build only after the backend-specific focused test and
   representative device validation pass. A successful CPU fallback is not
   proof that the GPU operation works.

## Symptom: geometry is rejected

1. Check for finite coordinates, `[longitude, latitude]` order where the input
   is GeoJSON, WGS84 bounds, supported geometry type, minimum line/ring
   vertices, ring closure, and polygon winding where that validator applies.
2. Confirm the declared CRS matches the numeric coordinates. The geo geometry
   types do not implicitly reproject data.
3. Use the returned error and field-level validation messages to correct the
   source payload. Do not clamp or silently repair rejected external geometry
   in an operator retry.
4. If the input is valid under the public contract but rejected by a particular
   path, retain a minimized redacted reproducer and the exact implementation
   and validator invoked; the contract and standalone `GeoValidator` have
   distinct limits (see [TRUST.md](./TRUST.md)).

## Symptom: spatial result is empty, incomplete, or unexpectedly broad

1. Confirm the query bbox and geometry coordinates use the same CRS and axis
   order. `MBR` operations are axis-aligned 2D comparisons.
2. Determine whether the path returns MBR candidates or applies an exact
   predicate. Candidate overlap alone does not establish geometry
   intersection.
3. For joins, record the predicate, thresholds, input counts, invalid-input
   policy, and pair cap. For temporal queries, note that the documented bbox
   and distance helpers select on geometry centroids.
4. Do not clear, reload, or mutate the index as a first response. Engage the
   index/storage owner to verify source data and cursor lifecycle, then use
   that owner's safe rebuild/recovery procedure.

## Symptom: raster request returns a non-OK status

Check `RasterStatus` before reading the result grid. `NOT_SUPPORTED` means the
feature is not compiled in; `INVALID_KEY`, `INVALID_BBOX`, and `TILE_TOO_LARGE`
require correcting the tile/bounds/request size; `BACKEND_ERROR` includes an
error message for escalation. Do not treat a default-constructed result as a
successful empty raster.

## Escalation packet

Send the geo and owning service maintainers the revision/build flags, redacted
request shape, CRS and units, error/status, device and backend reports,
relevant diagnostics/logs, test or benchmark reproducer, and whether fallback
was active. Escalate immediately when results differ between backends, invalid
input reaches a successful query path, a required CPU fallback is unavailable,
or a recovery would mutate index-owned data.

For normal health and capacity checks, use [OPERATIONS.md](./OPERATIONS.md).
For the threat and authorization boundary, use [TRUST.md](./TRUST.md) and
project-level [SECURITY.md](../../SECURITY.md).

## Sourcecode verification

- Diagnostic interfaces: [`spatial_backend.h`](../../include/geo/spatial_backend.h),
  [`geo_operator_diagnostics.h`](../../include/geo/geo_operator_diagnostics.h),
  [`geo_operator_diagnostics.cpp`](./geo_operator_diagnostics.cpp).
- Raster outcomes: [`raster_query_interface.h`](../../include/geo/raster_query_interface.h)
  and [`raster_query_interface.cpp`](./raster_query_interface.cpp).
- Backend build selection: [`CMakeLists.txt`](./CMakeLists.txt).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
